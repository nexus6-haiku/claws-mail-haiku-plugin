/*
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <AboutWindow.h>
#include <Application.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <SupportDefs.h>
#include <Roster.h>

#include "ConstantDefs.h"

class App : public BApplication
{
public:
				App(void);
	
	void 		ReadyToRun();
	bool		QuitRequested();
	void		AboutRequested();
	void		MessageReceived(BMessage *message);
	void		RemoveIconFromDeskbar();
	void		AddIconToDeskbar();
};
