/*
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef HAIKU_H
#define HAIKU_H


static gboolean my_folder_item_update_hook(gpointer, gpointer);
static gboolean my_folder_update_hook(gpointer, gpointer);
static gboolean my_msginfo_update_hook(gpointer, gpointer);
static gboolean my_offline_switch_hook(gpointer, gpointer);
static gboolean my_main_window_close_hook(gpointer, gpointer);
static gboolean my_main_window_got_iconified_hook(gpointer, gpointer);
static gboolean my_account_list_changed_hook(gpointer, gpointer);
static gboolean my_update_theme_hook(gpointer, gpointer);

void compose_mail();
void select_mail(char *file);

static gulong hook_f_item;
static gulong hook_f;
static gulong hook_m_info;
static gulong hook_offline;
static gulong hook_mw_close;
static gulong hook_got_iconified;
static gulong hook_account;
static gulong hook_theme_changed;

#endif // HAIKU_H
