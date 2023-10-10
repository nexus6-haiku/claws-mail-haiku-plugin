/*
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include <stdio.h>
#include <exception>

#include <Alert.h>
#include <Application.h>
#include <Path.h>
#include <WindowInfo.h>

#include "Handler.h"
#include "ConstantDefs.h"
#include "BeDC.h"
// #include "HaikuIntegration.h"


extern "C" {

	void compose_mail();
	void select_mail(const char *file);
	void DCLog(const char *text);

	Handler::Handler()
		:BHandler("spying handler")
	{
	}
			
	Handler::~Handler()
	{
		// DCLog("Handler::~Handler()");
	};
			
	void 
	Handler::MessageReceived(BMessage *msg)
	{
		BeDC dc(kApplicationName);
		switch (msg->what) {
			case COMPOSE_MESSAGE: {
				try {
					do_bring_to_front_team(BRect(), be_app->Team(), false);
					compose_mail();
					// DCLog("Handler::MessageReceived(COMPOSE_MESSAGE)");
				} catch(std::exception &e) {
					DCLog("Handler::MessageReceived(COMPOSE_MESSAGE) exception");
				}
			}
			break;
			case B_ARGV_RECEIVED: {
				try {
					// DCLog("Handler::MessageReceived(B_ARGV_RECEIVED)");
					dc.DumpBMessage(msg);
					auto param = msg->FindString("argv",1);
					select_mail(param);
				} catch(std::exception &e) {
					DCLog("Handler::MessageReceived(B_ARGV_RECEIVED) exception");
				}
			}
			break;
			case B_REFS_RECEIVED: {
				try {
					// DCLog("Handler::MessageReceived(B_REFS_RECEIVED)");
					dc.DumpBMessage(msg);
					entry_ref ref;
					if (msg->FindRef("refs", &ref) == B_OK) {
						// DCLog("Handler::MessageReceived(B_REFS_RECEIVED): Get ref");
						BEntry entry(&ref);
						BPath path;
						entry.GetPath(&path);
						// DCLog("Handler::MessageReceived(B_REFS_RECEIVED): open_mail");
						select_mail(path.Path());
					}
				} catch(std::exception &e) {
					DCLog("Handler::MessageReceived() exception");
				}
			}
			break;
		} 
		BHandler::MessageReceived(msg);
	}
	
	
	MessageFilter::MessageFilter(BHandler *target)
		:
		BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, nullptr),
		fTarget(target)
	{
	}
	
	MessageFilter::~MessageFilter() 
	{
		// DCLog("Handler::~MessageFilter()");
	}
	
	filter_result
	MessageFilter::Filter(BMessage* message, BHandler** target)
	{
		// BeDC().DumpBMessage(message, "Handler::MessageReceived()");
		switch (message->what) {
			case B_ARGV_RECEIVED:
			case B_REFS_RECEIVED:
			case COMPOSE_MESSAGE: {
				*target = fTarget;
			}
			break;
			default:
			break;
		}
		return B_DISPATCH_MESSAGE;
	}
}