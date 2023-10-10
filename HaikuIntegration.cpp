/*
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <Notification.h>
#include <OS.h>
#include <Roster.h>

#include "ConstantDefs.h"
#include "Handler.h"
#include "BeDC.h"


extern "C" 
{
	BHandler *_handler;
	BMessageFilter *_filter;
	
	BHandler *GetHandler(const char *name)
	{
		be_app->LockLooper();
		BHandler *target = nullptr;
		auto count = be_app->CountHandlers();
		for (int i = 0; i < count; i++) {
			auto handler = be_app->HandlerAt(i);
			BString handler_name(handler->Name());
			if (handler_name == name)
				target = handler;
		}
		return target;
	}

	void ShowAlert(const char *text)
	{
		BAlert("Haiku Plugin", text, "OK").Go();
	}
	
	void DCLog(const char *text)
	{
		thread_id this_thread = find_thread(NULL);
		BString str;
		str.SetToFormat("%s (current thread: %d)", text, this_thread);
		BeDC(kApplicationName).SendMessage(str.String());
	}
	
	void vDCLog(const char *text, va_list args)
	{
		BeDC dc(kApplicationName);
		dc.SendFormat(text, args);
	}
	
	void Init()
	{
		status_t status = BRoster().Launch(kApplicationSignature);
		if (status != B_OK) {
			BString error;
			error.SetToFormat("ClawsMailHaikuPlugin: Failed launching replicant app: %s\n", strerror(status));
			printf(error.String());
		}

		try {
			be_app->Lock();	
			
			_handler = new Handler();
			_filter = new MessageFilter(_handler);
			be_app->AddHandler(_handler);
			be_app->AddCommonFilter(_filter);
			
			be_app->Unlock();
		} catch(...) {}
	}

	void UnInit()
	{	
		try {
			be_app->Lock();
			
			be_app->RemoveCommonFilter(_filter);
			be_app->RemoveHandler(_handler);
			delete _handler;
			delete _filter;
			BMessenger(kApplicationSignature).SendMessage(B_QUIT_REQUESTED);
			
			be_app->Unlock();
		} catch(...) {}
	}
	
	void NotifyUpdateMessage(unsigned int _total_msgs, unsigned int _unread_msgs, unsigned int _new_msgs, 
								unsigned int _unreadmarked_msgs, unsigned int _marked_msgs)
	{
		auto message = new BMessage(UPDATE_STATISTICS);
		message->AddUInt16("total", _total_msgs);
		message->AddUInt16("unread", _unread_msgs);
		message->AddUInt16("unread_marked", _unreadmarked_msgs);
		message->AddUInt16("marked", _marked_msgs);
		message->AddUInt16("new", _new_msgs);		
		BMessenger(kApplicationSignature).SendMessage(message);
		// vDCLog("New %d, Unread: %d, Total: %d", _new_msgs, _unread_msgs, _total_msgs);
		// BeDC(kApplicationName).DumpBMessage(message);
	}
	
	void ShowNotification(const char *from, const char *subject, int32 msgnum, 
							const char *msgid, const char *id)
	{
		BString bid(id);
		BString newid;
		BString bmsgnum;
		bmsgnum << msgnum;
		
		newid = bid.RemoveAll(msgid);
		newid.Append(bmsgnum);
	
		BeDC(kApplicationName).SendFormat("NOTIFY: %s", newid.String());
		
		BNotification notification(B_INFORMATION_NOTIFICATION);
		notification.SetGroup("Claws Mail");
		notification.SetMessageID(newid);
		notification.SetTitle(from);
		notification.SetContent(subject);
		notification.AddOnClickArg(newid);
		notification.SetOnClickApp(kClawsMailSignature);
		notification.Send();
	}

}