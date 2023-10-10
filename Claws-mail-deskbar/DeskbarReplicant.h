/*
 * Copyright 2009-2021, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Weirauch, dev@m-phasis.de
 *		Humdinger, humdingerb@gmail.com
 * Adapted from Clipdinger by Nexus6 in 2023
 */
 
#pragma once

#include <AboutWindow.h>
#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <Entry.h>
#include <IconUtils.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <Roster.h>
#include <View.h>
#include <WindowInfo.h> // private header

#include "ConstantDefs.h"

class DeskbarReplicant : public BView {
	public:
						DeskbarReplicant(BRect frame, int32 resizingMode);
						DeskbarReplicant(BMessage* archive);
		virtual 		~DeskbarReplicant();

		static			DeskbarReplicant* Instantiate(BMessage* archive);
		virtual	status_t Archive(BMessage* archive, bool deep = true) const;

		virtual	void	AttachedToWindow();
		virtual void	DetachedFromWindow();
		void 			Pulse();

		virtual	void	Draw(BRect updateRect);
 
		virtual	void	MessageReceived(BMessage* msg);
		virtual	void	MouseDown(BPoint where);

	private:
		void			_Init();
		void			_ForwardMessage(const char* signature, BMessage *message);
		void			_QuitApplicationAndWait(const char* signature);
		void			_LaunchApplicationAndWait(const char* signature);

		BBitmap*		fIcon;
		
		BMessenger		fAppMessenger;
		
		uint32			fUnreadMessages;
		uint32			fTotalMessages;
		uint32			fUnreadMarkedMessages;
		uint32			fMarkedMessages;
		uint32			fNewMessages;
};