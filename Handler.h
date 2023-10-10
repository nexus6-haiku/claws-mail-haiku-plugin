/*
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <Handler.h>
#include <Message.h>
#include <MessageFilter.h>

extern "C" {
	class Handler : public BHandler {
	public:
								Handler();
		virtual					~Handler();
		
		void 					MessageReceived(BMessage *msg);
	};
	
	class MessageFilter : public BMessageFilter
	{
	public:
								MessageFilter(BHandler *target);
		virtual					~MessageFilter();

		virtual	filter_result	Filter(BMessage* message, BHandler** target);
	private:
		BHandler				*fTarget;
	};
}