/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd is copyright (C) 2002-2006 ChatSpike-Dev.
 *                       E-mail:
 *                <brain@chatspike.net>
 *                <Craig@chatspike.net>
 *
 * Written by Craig Edwards, Craig McLure, and others.
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#include "inspircd.h"
#include "users.h"
#include "commands.h"
#include "helperfuncs.h"
#include "commands/cmd_loadmodule.h"

extern InspIRCd* ServerInstance;

void cmd_loadmodule::Handle (char **parameters, int pcnt, userrec *user)
{
	if (ServerInstance->LoadModule(parameters[0]))
	{
		WriteOpers("*** NEW MODULE: %s",parameters[0]);
		WriteServ(user->fd,"975 %s %s :Module successfully loaded.",user->nick, parameters[0]);
	}
	else
	{
		WriteServ(user->fd,"974 %s %s :Failed to load module: %s",user->nick, parameters[0],ServerInstance->ModuleError());
	}
}
