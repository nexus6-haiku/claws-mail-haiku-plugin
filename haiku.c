/*
 * Claws Mail -- a GTK based, lightweight, and fast e-mail client
 * Copyright (C) 1999-2012 the Claws Mail Team
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
 */

#include "HaikuIntegration.h"

#include <glib.h>
#include <glib/gi18n.h>

#include "config.h"
#include "claws.h"
#include "file-utils.h"
#include "hooks.h"
#include "haiku.h"
#include "log.h"
#include "mainwindow.h"
#include "notification_core.h"
#include "plugin.h"
#include "summaryview.h"
#include "utils.h"
#include "version.h"

#define PLUGIN_NAME (_("Haiku"))

time_t last_time_checked;
sem_id semaphore;

static gboolean my_account_list_changed_hook(gpointer source, gpointer data)
{
  notification_update_msg_counts(NULL);
  return FALSE;
}

static gboolean my_update_theme_hook(gpointer source, gpointer data)
{
  notification_update_msg_counts(NULL);
  return FALSE;
}

static gboolean my_main_window_got_iconified_hook(gpointer source, gpointer data)
{
  notification_update_msg_counts(NULL);
  return FALSE;
}

static gboolean my_main_window_close_hook(gpointer source, gpointer data)
{
  notification_update_msg_counts(NULL);
  return FALSE;
}

static gboolean my_offline_switch_hook(gpointer source, gpointer data)
{
  notification_update_msg_counts(NULL);
  return FALSE;
}

static gboolean my_folder_item_update_hook(gpointer source, gpointer data)
{
	DCLog("my_folder_item_update_hook() start");
	FolderItemUpdateData *update_data = source;
	FolderType ftype;
	gchar *uistr;

	g_return_val_if_fail(source != NULL, FALSE);

	if (folder_has_parent_of_type(update_data->item, F_DRAFT))
		return FALSE;

	notification_update_msg_counts(NULL);

	/* Check if the folder types is to be notified about */
	ftype = update_data->item->folder->klass->type;
	uistr = update_data->item->folder->klass->uistr;
	if(!notify_include_folder_type(ftype, uistr))
		return FALSE;
	if(update_data->update_flags & F_ITEM_UPDATE_MSGCNT)
		notification_update_msg_notifications(update_data);
	
	DCLog("my_folder_item_update_hook() end");
	return FALSE; 
}

static gboolean my_folder_update_hook(gpointer source, gpointer data)
{
	FolderUpdateData *hookdata;

	g_return_val_if_fail(source != NULL, FALSE);
	hookdata = source;

	if(hookdata->update_flags & FOLDER_REMOVE_FOLDERITEM)
		notification_update_msg_counts(hookdata->item);
	else
		notification_update_msg_counts(NULL);

	return FALSE;
}


static gboolean my_msginfo_update_hook(gpointer source, gpointer data)
{
	return FALSE;
}


gint register_hooks()
{
	hook_f_item = hooks_register_hook(FOLDER_ITEM_UPDATE_HOOKLIST, 
										my_folder_item_update_hook, NULL);
	if(hook_f_item == 0) {
		DCLog("Failed to register folder item update hook");
		return -1;
	}

	hook_f = hooks_register_hook(FOLDER_UPDATE_HOOKLIST, 
									my_folder_update_hook, NULL);
	if(hook_f == 0) {
		DCLog("Failed to register folder update hook");
		hooks_unregister_hook(FOLDER_ITEM_UPDATE_HOOKLIST, hook_f_item);
		return -1;
	}


	hook_m_info = hooks_register_hook(MSGINFO_UPDATE_HOOKLIST, 
										my_msginfo_update_hook, NULL);
	if(hook_m_info == 0) {
		DCLog("Failed to register msginfo update hook");
		hooks_unregister_hook(FOLDER_ITEM_UPDATE_HOOKLIST, hook_f_item);
		hooks_unregister_hook(FOLDER_UPDATE_HOOKLIST, hook_f);
		return -1;
	}

	hook_offline = hooks_register_hook(OFFLINE_SWITCH_HOOKLIST, 
										my_offline_switch_hook, NULL);
	if(hook_offline == 0) {
		DCLog("Failed to register offline switch hook");
		hooks_unregister_hook(FOLDER_ITEM_UPDATE_HOOKLIST, hook_f_item);
		hooks_unregister_hook(FOLDER_UPDATE_HOOKLIST, hook_f);
		hooks_unregister_hook(MSGINFO_UPDATE_HOOKLIST, hook_m_info);
		return -1;
	}

	hook_mw_close = hooks_register_hook(MAIN_WINDOW_CLOSE, 
										my_main_window_close_hook, NULL);
	if(hook_mw_close == 0) {
		DCLog("Failed to register main window close");
		hooks_unregister_hook(FOLDER_ITEM_UPDATE_HOOKLIST, hook_f_item);
		hooks_unregister_hook(FOLDER_UPDATE_HOOKLIST, hook_f);
		hooks_unregister_hook(MSGINFO_UPDATE_HOOKLIST, hook_m_info);
		hooks_unregister_hook(OFFLINE_SWITCH_HOOKLIST, hook_offline);
		return -1;
	}

	hook_got_iconified = hooks_register_hook(MAIN_WINDOW_GOT_ICONIFIED,
												my_main_window_got_iconified_hook, NULL);
	if(hook_got_iconified == 0) {
		DCLog("Failed to register got iconified hook");
		hooks_unregister_hook(FOLDER_ITEM_UPDATE_HOOKLIST, hook_f_item);
		hooks_unregister_hook(FOLDER_UPDATE_HOOKLIST, hook_f);
		hooks_unregister_hook(MSGINFO_UPDATE_HOOKLIST, hook_m_info);
		hooks_unregister_hook(OFFLINE_SWITCH_HOOKLIST, hook_offline);
		hooks_unregister_hook(MAIN_WINDOW_CLOSE, hook_mw_close);
		return -1;
	}

	hook_account = hooks_register_hook(ACCOUNT_LIST_CHANGED_HOOKLIST,
										my_account_list_changed_hook, NULL);
	if (hook_account == 0) {
		DCLog("Failed to register account list changed");
		hooks_unregister_hook(FOLDER_ITEM_UPDATE_HOOKLIST, hook_f_item);
		hooks_unregister_hook(FOLDER_UPDATE_HOOKLIST, hook_f);
		hooks_unregister_hook(MSGINFO_UPDATE_HOOKLIST, hook_m_info);
		hooks_unregister_hook(OFFLINE_SWITCH_HOOKLIST, hook_offline);
		hooks_unregister_hook(MAIN_WINDOW_CLOSE, hook_mw_close);
		hooks_unregister_hook(MAIN_WINDOW_GOT_ICONIFIED, hook_got_iconified);
		return -1;
	}

	hook_theme_changed = hooks_register_hook(THEME_CHANGED_HOOKLIST, 
												my_update_theme_hook, NULL);
	if(hook_theme_changed == 0) {
		DCLog("Failed to register theme change");
		hooks_unregister_hook(FOLDER_ITEM_UPDATE_HOOKLIST, hook_f_item);
		hooks_unregister_hook(FOLDER_UPDATE_HOOKLIST, hook_f);
		hooks_unregister_hook(MSGINFO_UPDATE_HOOKLIST, hook_m_info);
		hooks_unregister_hook(OFFLINE_SWITCH_HOOKLIST, hook_offline);
		hooks_unregister_hook(MAIN_WINDOW_CLOSE, hook_mw_close);
		hooks_unregister_hook(MAIN_WINDOW_GOT_ICONIFIED, hook_got_iconified);
		hooks_unregister_hook(ACCOUNT_LIST_CHANGED_HOOKLIST, hook_account);
		return -1;
	}
}

gint plugin_init(gchar **error)
{
	if (!check_plugin_version(MAKE_NUMERIC_VERSION(2,9,2,72),
				VERSION_NUMERIC, PLUGIN_NAME, error))
		return -1;
		
	if ((semaphore = create_sem(1, "semaphore")) < B_NO_ERROR)
		DCLog("Error creating semaphore");
		
	notification_notified_hash_startup_init();
	last_time_checked = time(NULL);
	// DCLog("plugin_init: last_time_checked");
	// DCLog(ctime(&last_time_checked));
	Init();
	register_hooks();
	DCLog("Haiku plugin loaded");
	return 0;
}


gboolean plugin_done(void)
{
	UnInit();
	notification_core_free();
	
	delete_sem(semaphore);
	
	DCLog("Haiku plugin unloaded");
	return TRUE;
}

const gchar *plugin_name(void)
{
	return PLUGIN_NAME;
}

const gchar *plugin_desc(void)
{
	return _("This plugin provides Haiku integration for Claws Mail. "
	         "\n\n"
	         "(c) 2023 Nexus6");
}

const gchar *plugin_type(void)
{
	return "Common";
}

const gchar *plugin_licence(void)
{
	return "GPL3+";
}

const gchar *plugin_version(void)
{
	return "0.1";
}

struct PluginFeature *plugin_provides(void)
{
	static struct PluginFeature features[] = 
		{ {PLUGIN_OTHER, N_("Haiku")},
		  {PLUGIN_NOTHING, NULL}};
	return features;
}

gboolean do_compose_mail()
{
	DCLog("compose_mail()");
	MainWindow *mainwin = mainwindow_get_mainwindow();
	compose_mail_cb(mainwin, 0, NULL);
	return FALSE;
}

void compose_mail()
{
	g_main_context_invoke(NULL, (GSourceFunc)do_compose_mail, NULL);
}

gboolean do_select_mail(char *file)
{
	DCLog(file);
	mainwindow_jump_to(file, TRUE);
	MainWindow *mainwin = mainwindow_get_mainwindow();
	// summary_open_msg(mainwin->summaryview);
	return FALSE;
}

void select_mail(char *file)
{
	g_main_context_invoke(NULL, (GSourceFunc)do_select_mail, file);
}
