/*
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "App.h"
#include <Alert.h>
#include <Message.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Application"

App::App(void)
	:	BApplication(kApplicationSignature)
{
}
	
void 
App::ReadyToRun()
{
	AddIconToDeskbar();
}

bool 
App::QuitRequested()
{
	RemoveIconFromDeskbar();
	BApplication::QuitRequested();
	return true;
}

void
App::AboutRequested()
{
	BAboutWindow* aboutW
		= new BAboutWindow(B_TRANSLATE_SYSTEM_NAME(kApplicationName), kApplicationSignature);
	aboutW->AddDescription(B_TRANSLATE("A Deskbar replicant for Claws Mail\n"));
	aboutW->AddCopyright(2023, "Nexus6");
	aboutW->Show();
}

void
App::MessageReceived(BMessage *message)
{
	switch(message->what) {
		case UPDATE_STATISTICS:
		{
			SendNotices(message->what, message);
		}
	}
	BApplication::MessageReceived(message);
}

void
App::RemoveIconFromDeskbar()
{
	BDeskbar deskbar;
	int32 found_id;

	if (deskbar.GetItemInfo(kApplicationName, &found_id) == B_OK) {
		status_t err = deskbar.RemoveItem(found_id);
		if (err != B_OK) {
			printf("Error removing replicant id "
				   "%" B_PRId32 ": %s\n",
				found_id, strerror(err));
		}
	}
}

void
App::AddIconToDeskbar()
{
	app_info appInfo;
	status_t status = be_app->GetAppInfo(&appInfo);
	if (status != B_OK)
		printf("Failed getting app info: %s\n", strerror(status));

	BDeskbar deskbar;

	if (!deskbar.IsRunning())
		return;

	if (deskbar.HasItem(kApplicationName))
		RemoveIconFromDeskbar();

	int32 id;
	appInfo.ref.set_name(kApplicationName);
	status_t res = deskbar.AddItem(&appInfo.ref, &id);
	if (res != B_OK)
		printf("Failed adding deskbar icon: %s\n", strerror(res));
}