/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  Inspire is copyright (C) 2002-2004 ChatSpike-Dev.
 *                       E-mail:
 *                <brain@chatspike.net>
 *           	  <Craig@chatspike.net>
 *     
 * Written by Craig Edwards, Craig McLure, and others.
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 */

// Globops and +g support module by C.J.Edwards

#include <stdio.h>
#include <string>
#include "users.h"
#include "channels.h"
#include "modules.h"

/* $ModDesc: Provides support for unreal-style GLOBOPS and umode +g */

Server *Srv;
	 
void handle_globops(char **parameters, int pcnt, userrec *user)
{
	std::string line = "*** GLOBOPS - From " + std::string(user->nick) + ": ";
	for (int i = 0; i < pcnt; i++)
	{
		line = line + std::string(parameters[i]) + " ";
	}
	Srv->SendToModeMask("og",WM_AND,line);
}


class ModuleGlobops : public Module
{
 public:
	ModuleGlobops()
	{
		Srv = new Server;
		
		if (!Srv->AddExtendedMode('g',MT_CLIENT,true,0,0))
		{
			Srv->Log(DEFAULT,"*** m_globops: ERROR, failed to allocate user mode +g!");
			printf("Could not claim usermode +g for this module!");
			exit(0);
		}
		Srv->AddCommand("GLOBOPS",handle_globops,'o',1);
	}
	
	virtual ~ModuleGlobops()
	{
		delete Srv;
	}
	
	virtual Version GetVersion()
	{
		return Version(1,0,0,1);
	}
	
	virtual int OnExtendedMode(userrec* user, void* target, char modechar, int type, bool mode_on, string_list &params)
	{
		// check if this is our mode character...
		if ((modechar == 'g') && (type == MT_CLIENT))
  		{
			// we dont actually do anything with the mode in this module -
   			// just tell the core its been claimed and is ok to give users.  			
			return 1;
		}
		else
		{
			// this mode isn't ours, we have to bail and return 0 to not handle it.
			return 0;
		}
	}

	virtual void OnOper(userrec* user)
	{
		char* modes[2];			// only two parameters
		modes[0] = user->nick;		// first parameter is the nick
		modes[1] = "+g";		// second parameter is the mode
		Srv->SendMode(modes,2,user);	// send these, forming the command "MODE <nick> +g"
	}

};

// stuff down here is the module-factory stuff. For basic modules you can ignore this.

class ModuleGlobopsFactory : public ModuleFactory
{
 public:
	ModuleGlobopsFactory()
	{
	}
	
	~ModuleGlobopsFactory()
	{
	}
	
	virtual Module * CreateModule()
	{
		return new ModuleGlobops;
	}
	
};


extern "C" void * init_module( void )
{
	return new ModuleGlobopsFactory;
}

