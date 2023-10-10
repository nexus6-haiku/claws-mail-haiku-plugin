/* Notification plugin for Claws Mail
 * Copyright (C) 2005-2007 Holger Berndt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Modified by Nexus6 nexus6.haiku@icloud.com in 2023 to work with the Haiku plugin
 */

#include "config.h"
#include "claws-features.h"
#include "folder.h"
#include "folderview.h"
#include "codeconv.h"
#include "gtk/gtkutils.h"

#include "haiku.h"
#include "HaikuIntegration.h"
#include "notification_core.h"



static gchar *defstr (gchar *s)
{
	return s ? s : "(null)";
} /* defstr */

void notification_update_trayicon()
{
	// gchar *buf;
	NotificationMsgCount count;
	GSList *list;

	notification_core_get_msg_count(list, &count);

	// gchar str[256];
	// sprintf(str, "New %d, Unread: %d, Total: %d",
			// count.new_msgs, count.unread_msgs,
			// count.total_msgs);
	// DCLog(str);
	// g_free(buf);
	
	NotifyUpdateMessage(count.total_msgs, count.unread_msgs, count.new_msgs, 
								count.unreadmarked_msgs, count.marked_msgs);
}

void notification_update_msg_counts(FolderItem *removed_item)
{
	if(!msg_count_hash)
	msg_count_hash = g_hash_table_new_full(g_str_hash,g_str_equal, g_free,g_free);

	folder_func_to_all_folders(msg_count_hash_update_func, msg_count_hash);

	if(removed_item) {
		gchar *identifier;
		identifier = folder_item_get_identifier(removed_item);
		if(identifier) {
			g_hash_table_remove(msg_count_hash, identifier);
			g_free(identifier);
		}
	}
	msg_count_clear(&msg_count);
	g_hash_table_foreach(msg_count_hash, msg_count_update_from_hash, NULL);
	notification_update_trayicon();
}

static void msg_count_clear(NotificationMsgCount *count)
{
  count->new_msgs          = 0;
  count->unread_msgs       = 0;
  count->unreadmarked_msgs = 0;
  count->marked_msgs       = 0;
  count->total_msgs        = 0;
}

/* c1 += c2 */
static void msg_count_add(NotificationMsgCount *c1,NotificationMsgCount *c2)
{
  c1->new_msgs          += c2->new_msgs;
  c1->unread_msgs       += c2->unread_msgs;
  c1->unreadmarked_msgs += c2->unreadmarked_msgs;
  c1->marked_msgs       += c2->marked_msgs;
  c1->total_msgs        += c2->total_msgs;
}

/* c1 = c2 */
static void msg_count_copy(NotificationMsgCount *c1,NotificationMsgCount *c2)
{
  c1->new_msgs          = c2->new_msgs;
  c1->unread_msgs       = c2->unread_msgs;
  c1->unreadmarked_msgs = c2->unreadmarked_msgs;
  c1->marked_msgs       = c2->marked_msgs;
  c1->total_msgs        = c2->total_msgs;
}

gboolean get_flat_gslist_from_nodes_traverse_func(GNode *node, gpointer data)
{
  if(node->data) {
    GSList **list = data;
    *list = g_slist_prepend(*list, node->data);
  }
  return FALSE;
}

GSList* get_flat_gslist_from_nodes(GNode *node)
{
  GSList *retval = NULL;

  g_node_traverse(node, G_PRE_ORDER, G_TRAVERSE_ALL, -1, get_flat_gslist_from_nodes_traverse_func, &retval);
  return retval;
}

void notification_core_get_msg_count_of_foldername(gchar *foldername, NotificationMsgCount *count)
{
  GList *list;
  GSList *f_list;

  Folder *walk_folder;
  Folder *folder = NULL;

  for(list = folder_get_list(); list != NULL; list = list->next) {
    walk_folder = list->data;
    if(g_strcmp0(foldername, walk_folder->name) == 0) {
      folder = walk_folder;
      break;
    }
  }
  if(!folder) {
    debug_print("Notification plugin: Error: Could not find folder %s\n", foldername);
    return;
  }

  msg_count_clear(count);
  f_list = get_flat_gslist_from_nodes(folder->node);
  notification_core_get_msg_count(f_list, count);
  g_slist_free(f_list);
}

void notification_core_get_msg_count(GSList *folder_list,
				     NotificationMsgCount *count)
{
	GSList *walk;

	if(!folder_list)
		msg_count_copy(count,&msg_count);
	else {
		msg_count_clear(count);
		for(walk = folder_list; walk; walk = walk->next) {
			gchar *identifier;
			NotificationMsgCount *item_count;
			FolderItem *item = (FolderItem*) walk->data;
			identifier = folder_item_get_identifier(item);
			if(identifier) {
				item_count = g_hash_table_lookup(msg_count_hash,identifier);
				g_free(identifier);
				if(item_count)
					msg_count_add(count, item_count);
			}
		}
	}
}

static void msg_count_hash_update_func(FolderItem *item, gpointer data)
{
  gchar *identifier;
  NotificationMsgCount *count;
  GHashTable *hash = data;

  if(!notify_include_folder_type(item->folder->klass->type,
				 item->folder->klass->uistr))
    return;

  identifier = folder_item_get_identifier(item);
  if(!identifier)
    return;

  count = g_hash_table_lookup(hash, identifier);

  if(!count) {
    count = g_new0(NotificationMsgCount,1);
    g_hash_table_insert(hash, identifier, count);
  }
  else
    g_free(identifier);

  count->new_msgs          = item->new_msgs;
  count->unread_msgs       = item->unread_msgs;
  count->unreadmarked_msgs = item->unreadmarked_msgs;
  count->marked_msgs       = item->marked_msgs;
  count->total_msgs        = item->total_msgs;
}

static void msg_count_update_from_hash(gpointer key, gpointer value,
				       gpointer data)
{
  NotificationMsgCount *count = value;
  msg_count_add(&msg_count,count);
}

/* Replacement for the post-filtering hook:
   Pseudocode by Colin:
hook on FOLDER_ITEM_UPDATE_HOOKLIST
 if hook flags & F_ITEM_UPDATE_MSGCOUNT
  scan mails (folder_item_get_msg_list)
   if MSG_IS_NEW(msginfo->flags) and not in hashtable
    notify()
    add to hashtable
   procmsg_msg_list_free

hook on MSGINFO_UPDATE_HOOKLIST
 if hook flags & MSGINFO_UPDATE_FLAGS
  if !MSG_IS_NEW(msginfo->flags)
   remove from hashtable, it's now useless
*/

/* On startup, mark all new mails as already notified
 * (by including them in the hash) */
void notification_notified_hash_startup_init(void)
{
	// GList *folder_list, *walk;
	// Folder *folder;

	if(!notified_hash) {
		notified_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
		DCLog("Notification Plugin: notified messages Hash table created\n");
	}
	
	// folder_list = folder_get_list();
	// for(walk = folder_list; walk != NULL; walk = g_list_next(walk)) {
		// folder = walk->data;
		// g_node_traverse(folder->node, G_PRE_ORDER, G_TRAVERSE_ALL, -1,
						// notification_traverse_hash_startup, NULL);
	// }
}

void notification_core_free(void)
{
	if(notified_hash) {
		g_hash_table_destroy(notified_hash);
		notified_hash = NULL;
	}
	if(msg_count_hash) {
		g_hash_table_destroy(msg_count_hash);
		msg_count_hash = NULL;
	}
	
	DCLog("Notification Plugin: Freed internal data\n");
}

static gboolean notification_traverse_collect(GNode *node, gpointer data)
{
  TraverseCollect *cdata = data;
  FolderItem *item = node->data;
  gchar *folder_id_cur;

  /* Obey global folder type limitations */
  if(!notify_include_folder_type(item->folder->klass->type,
				 item->folder->klass->uistr))
    return FALSE;

  /* If a folder_items list was given, check it first */
  if((cdata->folder_items) && (item->path != NULL) &&
     ((folder_id_cur  = folder_item_get_identifier(item)) != NULL)) {
    FolderItem *list_item;
    GSList *walk;
    gchar *folder_id_list;
    gboolean eq;
    gboolean folder_in_list = FALSE;

    for(walk = cdata->folder_items; walk != NULL; walk = g_slist_next(walk)) {
      list_item = walk->data;
      folder_id_list = folder_item_get_identifier(list_item);
      eq = !g_strcmp0(folder_id_list,folder_id_cur);
      g_free(folder_id_list);
      if(eq) {
	folder_in_list = TRUE;
	break;
      }
    }
    g_free(folder_id_cur);
    if(!folder_in_list)
      return FALSE;
  }

  if(item->new_msgs || (cdata->unread_also && item->unread_msgs)) {
    GSList *msg_list = folder_item_get_msg_list(item);
    GSList *walk;
    for(walk = msg_list; walk != NULL; walk = g_slist_next(walk)) {
      MsgInfo *msg_info = walk->data;
      CollectedMsg *cmsg;

      if((cdata->max_msgs != 0) && (cdata->num_msgs >= cdata->max_msgs))
	return FALSE;

      if(MSG_IS_NEW(msg_info->flags) ||
				 (MSG_IS_UNREAD(msg_info->flags) && cdata->unread_also)) {

				cmsg = g_new(CollectedMsg, 1);
				cmsg->from = g_strdup(msg_info->from ? msg_info->from : "");
				cmsg->subject = g_strdup(msg_info->subject ? msg_info->subject : "");
				if(msg_info->folder && msg_info->folder->name)
					cmsg->folderitem_name = g_strdup(msg_info->folder->path);
				else
					cmsg->folderitem_name = g_strdup("");

				cmsg->msginfo = msg_info;

				cdata->collected_msgs = g_slist_prepend(cdata->collected_msgs, cmsg);
				cdata->num_msgs++;
      }
    }
    procmsg_msg_list_free(msg_list);
  }

  return FALSE;
}

gboolean notify_include_folder_type(FolderType ftype, gchar *uistr)
{
	gboolean retval = FALSE;
	
	switch(ftype) {
		case F_MH:
		case F_MBOX:
		case F_MAILDIR:
		case F_IMAP:
			// vDCLog("notify_include_folder_type(): uistr = %s", uistr);
			retval = TRUE;
		break;
		case F_NEWS:
			retval = TRUE;
		break;
		case F_UNKNOWN:
			if(uistr == NULL)
				retval = FALSE;
			else if(!strcmp(uistr, "vCalendar")) {
				retval = FALSE;
			} else if(!strcmp(uistr, "RSSyl")) {
				retval = FALSE;
			} else
			  vDCLog("Notification Plugin: Unknown folder type %d\n",ftype);
		break;
		default:
			vDCLog("Notification Plugin: Unknown folder type %d\n",ftype);
	}

	return retval;
}

gboolean is_message_new(MsgInfo *msginfo)
{	
	// if (MSG_IS_UNREAD(msginfo->flags))
		// vDCLog("%s unread", msginfo->subject);
	
	if (MSG_IS_NEW(msginfo->flags))
		vDCLog("%s new", msginfo->subject);

	if (msginfo->date_t >= last_time_checked)
		vDCLog("%s recent", msginfo->subject);
		
	gboolean retvalue = FALSE;
	if (MSG_IS_NEW(msginfo->flags)) {
		vDCLog("%s is new", msginfo->subject);
		retvalue = TRUE;
	} else {
		time_t msgtime = msginfo->date_t;
		if ((msginfo->date_t >= last_time_checked)) { // && MSG_IS_UNREAD(msginfo->flags)) {
			// DCLog("is_message_new: last_time_checked");
			// DCLog(ctime(&last_time_checked));
			// vDCLog("msgtime: %s", ctime(&msgtime));
			// vDCLog("%s is recent and unread", msginfo->subject);
			vDCLog("%s is recent", msginfo->subject);
			retvalue = TRUE;
		} else {
			retvalue = FALSE;
		}
	}
	return retvalue;
}

void notification_update_msg_notifications(FolderItemUpdateData *update_data)
{
	// vDCLog("notification_update_msg_notifications(): folder = %s", update_data->item->folder->name);
	// vDCLog("notification_update_msg_notifications(): last_time_checked = %s", ctime(&last_time_checked));
	
	GSList *msg_list, *walk;
	
	if (notified_hash == NULL)
		return;
	
	msg_list = folder_item_get_msg_list(update_data->item);

	for(walk = msg_list; walk; walk = g_slist_next(walk)) {
		// DCLog("notification_new_unnotified_msgs: ciclo for");
		MsgInfo *msginfo;
		gchar *msgid;
		
		msginfo = (MsgInfo*) walk->data;
		if (!msginfo)
			goto exit;
						
		msgid = msginfo->msgid;// ? msginfo->msgid : "";
			
		if(is_message_new(msginfo)) {
			
			gchar *folder_id;
			gchar *id;
			id = procmsg_msginfo_get_identifier(msginfo);
					
			if (acquire_sem_etc(semaphore, 1, B_RELATIVE_TIMEOUT, 500000) == B_NO_ERROR) {
				if(g_hash_table_lookup(notified_hash, id) != NULL) {
					vDCLog("Skip. msg %s already notified", msginfo->subject);
					// goto exit;
				} else {
					ShowNotification(defstr(msginfo->from), defstr(msginfo->subject), msginfo->msgnum,
								defstr(msginfo->msgid), defstr(id));
					g_hash_table_insert(notified_hash, g_strdup(id), GINT_TO_POINTER(1));
				}
				release_sem(semaphore);
			}

			// if (msginfo->date_t > last_time_checked)
				// last_time_checked = msginfo->date_t;
		}
	}
	
exit:
	procmsg_msg_list_free(msg_list);
	return;
}