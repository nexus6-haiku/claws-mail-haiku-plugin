/*
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
// include Haiku stuff for semaphores instead of glib mutex
#include <OS.h>
 
#pragma once

void ShowAlert(const char *text);
void Init();
void UnInit();
void DCLog(const char *text);
void vDCLog(const char *text, ...);
long int GetTime();
void ShowNotification(const char *from, const char *subject, int32 msgnum, 	
						const char *msgid, const char *id);
