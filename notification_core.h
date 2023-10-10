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
 */

#ifndef NOTIFICATION_CORE_H
#define NOTIFICATION_CORE_H NOTIFICATION_CORE_H

#include "mainwindow.h"
#include "folder.h"
#include "procmsg.h"

typedef struct {
	gchar *from;
	gchar *subject;
	FolderItem *folder_item;
	gchar *folderitem_name;
	MsgInfo *msginfo;
} CollectedMsg;

typedef enum {
  F_TYPE_MAIL=0,
  F_TYPE_NEWS,
  F_TYPE_CALENDAR,
  F_TYPE_RSS,
  F_TYPE_LAST
} NotificationFolderType;

typedef struct 
{
	guint new_msgs;
	guint unread_msgs;
	guint unreadmarked_msgs;
	guint marked_msgs;
	guint total_msgs;
} NotificationMsgCount;

extern time_t last_time_checked;
extern sem_id semaphore;

typedef struct {
	GSList *collected_msgs;
	GSList *folder_items;
	gboolean unread_also;
	gint max_msgs;
	gint num_msgs;
} TraverseCollect;

static GHashTable *msg_count_hash;
static NotificationMsgCount msg_count;
static GHashTable *notified_hash;

void DCLog(const char *test);
void vDCLog(const char *test, ...);

void     	notification_core_free(void);
void     	notification_update_msg_counts(FolderItem*);
void     	notification_core_get_msg_count(GSList*,NotificationMsgCount*);
void     	notification_core_get_msg_count_of_foldername(gchar*, NotificationMsgCount*);
void     	notification_notified_hash_startup_init(void);
void 		notification_update_msg_notifications(FolderItemUpdateData *update_data);
gboolean 	notify_include_folder_type(FolderType, gchar*);
static gboolean is_message_new(MsgInfo *msginfo);
static void msg_notify_from_hash(gpointer key, gpointer value, gpointer data);
static void msg_count_hash_update_func(FolderItem*, gpointer);
static void msg_count_update_from_hash(gpointer, gpointer, gpointer);
static void msg_count_clear(NotificationMsgCount*);
static void msg_count_add(NotificationMsgCount*,NotificationMsgCount*);
static void msg_count_copy(NotificationMsgCount*,NotificationMsgCount*);
void NotifyUpdateMessage(unsigned int total_msgs, unsigned int unread_msgs, unsigned int new_msgs, 
							unsigned int unreadmarked_msgs, unsigned int marked_msgs);

#endif /* NOTIFICATION_CORE_H */
