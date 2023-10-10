/*
 * Copyright 2018, Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _H
#define _H

static const char kApplicationName[] = "Claws-Mail-Deskbar";
static const char kApplicationSignature[] = "application/x-vnd.Claws-Mail-Deskbar";
static const char kClawsMailSignature[] = "application/x-vnd.claws-mail";
static const char kPortName[] = "ClawsMailHaikuPluginPort";

static const int OPEN_CLAWS = 'opcl';
static const int COMPOSE_MESSAGE = 'cmpm';
static const int UPDATE_STATISTICS = 'cmus';

extern void _RemoveIconFromDeskbar();

#endif // _H
