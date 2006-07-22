/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd is copyright (C) 2002-2004 ChatSpike-Dev.
 *		       E-mail:
 *		<brain@chatspike.net>
 *	   	  <Craig@chatspike.net>
 *     
 * Written by Craig Edwards, Craig McLure, and others.
 * This program is free but copyrighted software; see
 *	    the file COPYING for details.
 *
 * ---------------------------------------------------
 */

using namespace std;

#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include "users.h"
#include "channels.h"
#include "modules.h"
#include "inspircd.h"
#include "m_sqlv2.h"


static Module* SQLModule;
static Module* MyMod;
static std::string dbid;
extern time_t TIME;

enum LogTypes { LT_OPER = 1, LT_KILL, LT_SERVLINK, LT_XLINE, LT_CONNECT, LT_DISCONNECT, LT_FLOOD, LT_LOADMODULE };

enum QueryState { FIND_SOURCE, INSERT_SOURCE, FIND_NICK, INSERT_NICK, FIND_HOST, INSERT_HOST, INSERT_LOGENTRY, DONE };

class QueryInfo;

std::map<unsigned long,QueryInfo*> active_queries;

class QueryInfo
{
 public:
	QueryState qs;
	unsigned long id;
	std::string nick;
	std::string hostname;
	int sourceid;
	int nickid;
	int hostid;
	int category;
	time_t date;

	QueryInfo(const std::string &n, const std::string &h, unsigned long i, int cat)
	{
		qs = FIND_SOURCE;
		nick = n;
		hostname = h;
		id = i;
		category = cat;
		sourceid = nickid = hostid = -1;
		date = TIME;
	}

	void Go(SQLresult* res)
	{
		// Nothing here and not sent yet
		SQLrequest req = SQLreq(MyMod, SQLModule, dbid, "", "");

		log(DEBUG,"State: %d",qs);

		switch (qs)
		{
			case FIND_SOURCE:
				// "SELECT id,actor FROM ircd_log_actors WHERE actor='"+nick+"'"
				// If we find it, advance to FIND_NICK state, otherwise go to INSERT_SOURCE
				if (res->Rows())
				{
					qs = INSERT_SOURCE;
				}
				else
				{
					req = SQLreq(MyMod, SQLModule, dbid, "SELECT id,actor FROM ircd_log_actors WHERE actor='?'", nick);
					if(req.Send())
					{
						qs = FIND_SOURCE;
						active_queries[req.id] = this;
					}
					else
					{
						log(DEBUG, "SQLrequest failed: %s", req.error.Str());
					}
					break;

				}
			case INSERT_SOURCE:
				// "INSERT INTO ircd_log_actors VALUES('','"+nick+"')")
				// after we've done this, go back to FIND_SOURCE
				req = SQLreq(MyMod, SQLModule, dbid, "INSERT INTO ircd_log_actors VALUES('','?')", nick);
				if(req.Send())
				{
					qs = FIND_NICK;
					active_queries[req.id] = this;
				}
				else
				{
					log(DEBUG, "SQLrequest failed: %s", req.error.Str());
				}
				
			break;
			case FIND_NICK:
				// "SELECT id,actor FROM ircd_log_actors WHERE actor='"+nick+"'"
				// If we find it, advance to FIND_HOST state, otherwise go to INSERT_NICK
				if (res->Rows())
				{
					qs = INSERT_NICK;
				}
				else
				{
					req = SQLreq(MyMod, SQLModule, dbid, "SELECT id,actor FROM ircd_log_actors WHERE actor='?'", nick);
					if(req.Send())
					{
						qs = FIND_NICK;
						active_queries[req.id] = this;
					}
					else
					{
						log(DEBUG, "SQLrequest failed: %s", req.error.Str());
					}
					break;
				}
			case INSERT_NICK:
				// "INSERT INTO ircd_log_actors VALUES('','"+nick+"')")
				// after we've done this, go back to FIND_NICK
				req = SQLreq(MyMod, SQLModule, dbid, "INSERT INTO ircd_log_actors VALUES('','?')",nick);
				if(req.Send())
				{
					qs = FIND_HOST;
					active_queries[req.id] = this;
				}
				else
				{
					log(DEBUG, "SQLrequest failed: %s", req.error.Str());
				}
			break;
			case FIND_HOST:
				// "SELECT id,hostname FROM ircd_log_hosts WHERE hostname='"+host+"'"
				// If we find it, advance to INSERT_LOGENTRY state, otherwise go to INSERT_HOST
				if (res->Rows())
				{
					qs = INSERT_HOST;
				}
				else
				{
					req = SQLreq(MyMod, SQLModule, dbid, "SELECT id,hostname FROM ircd_log_hosts WHERE hostname='?'",hostname);
					if(req.Send())
					{
						qs = FIND_HOST;
						active_queries[req.id] = this;
					}
					else
					{
						log(DEBUG, "SQLrequest failed: %s", req.error.Str());
					}
					break;
				}
			case INSERT_HOST:
				// "INSERT INTO ircd_log_hosts VALUES('','"+host+"')"
				// after we've done this, go back to FIND_HOST
				req = SQLreq(MyMod, SQLModule, dbid, "INSERT INTO ircd_log_hosts VALUES('','?')", hostname);
				if(req.Send())
				{
					qs = INSERT_LOGENTRY;
					active_queries[req.id] = this;
				}
				else
				{
					log(DEBUG, "SQLrequest failed: %s", req.error.Str());
				}
			break;
			case INSERT_LOGENTRY:
				// INSERT INTO ircd_log VALUES('',%lu,%lu,%lu,%lu,%lu)",(unsigned long)category,(unsigned long)nickid,(unsigned long)hostid,(unsigned long)sourceid,(unsigned long)date
				// aaand done! (discard result)
				if ((nickid == -1) || (hostid == -1) || (sourceid == -1))
				{
					qs = FIND_SOURCE;
					this->Go(res);
				}
				else
				{
					req = SQLreq(MyMod, SQLModule, dbid, "INSERT INTO ircd_log VALUES('',"+ConvToStr(category)+","+ConvToStr(nickid)+","+ConvToStr(hostid)+","+ConvToStr(sourceid)+","+ConvToStr(date)+")");
							/*,category,
							nickid,
							hostid,
							sourceid,
							date);*/
					if(req.Send())
					{
						qs = DONE;
					}
					else
					{
						log(DEBUG, "SQLrequest failed: %s", req.error.Str());
					}
				}
			break;
			case DONE:
				active_queries[req.id] = NULL;
			break;
		}
	}
};

/* $ModDesc: Logs network-wide data to an SQL database */

class ModuleSQLLog : public Module
{
	Server* Srv;
	ConfigReader* Conf;

 public:
	bool ReadConfig()
	{
		Conf = new ConfigReader();
		dbid = Conf->ReadValue("sqllog","dbid",0);	// database id of a database configured in sql module
		DELETE(Conf);
		SQLModule = Srv->FindFeature("SQL");
		if (!SQLModule)
			Srv->Log(DEFAULT,"WARNING: m_sqllog.so could not initialize because an SQL module is not loaded. Load the module and rehash your server.");
		return (SQLModule);
	}

	ModuleSQLLog(Server* Me) : Module::Module(Me)
	{
		Srv = Me;
		ReadConfig();
		MyMod = this;
		active_queries.clear();
	}

	void Implements(char* List)
	{
		List[I_OnRehash] = List[I_OnOper] = List[I_OnGlobalOper] = List[I_OnKill] = 1;
		List[I_OnPreCommand] = List[I_OnUserConnect] = List[I_OnGlobalConnect] = 1;
		List[I_OnUserQuit] = List[I_OnLoadModule] = List[I_OnRequest] = 1;
	}

	virtual void OnRehash(const std::string &parameter)
	{
		ReadConfig();
	}

	virtual char* OnRequest(Request* request)
	{
		if(strcmp(SQLRESID, request->GetData()) == 0)
		{
			SQLresult* res;
			std::map<unsigned long, QueryInfo*>::iterator n;

			res = static_cast<SQLresult*>(request);
			log(DEBUG, "Got SQL result (%s) with ID %lu", res->GetData(), res->id);

			n = active_queries.find(res->id);

			if (n != active_queries.end())
			{
				log(DEBUG,"This is an active query");
				n->second->Go(res);

				std::map<unsigned long, QueryInfo*>::iterator n = active_queries.find(res->id);
				active_queries.erase(n);
			}
		}
		return SQLSUCCESS;
	}

	void AddLogEntry(int category, const std::string &nick, const std::string &host, const std::string &source)
	{
		// is the sql module loaded? If not, we don't attempt to do anything.
		if (!SQLModule)
			return;

		SQLrequest req = SQLreq(this, SQLModule, dbid, "SELECT id,actor FROM ircd_log_actors WHERE actor='?'", nick);
		if(req.Send())
		{
			QueryInfo* i = new QueryInfo(nick, host, req.id, category);
			i->qs = FIND_SOURCE;
			active_queries[req.id] = i;
		}
		else
		{
			log(DEBUG, "SQLrequest failed: %s", req.error.Str());
		}

		/*long nickid = InsertNick(nick);
		long sourceid = InsertNick(source);
		long hostid = InsertHost(host);
		InsertEntry((unsigned)category,(unsigned)nickid,(unsigned)hostid,(unsigned)sourceid,(unsigned long)time(NULL));*/
	}

	virtual void OnOper(userrec* user, const std::string &opertype)
	{
		AddLogEntry(LT_OPER,user->nick,user->host,user->server);
	}

	virtual void OnGlobalOper(userrec* user)
	{
		AddLogEntry(LT_OPER,user->nick,user->host,user->server);
	}

	virtual int OnKill(userrec* source, userrec* dest, const std::string &reason)
	{
		AddLogEntry(LT_KILL,dest->nick,dest->host,source->nick);
		return 0;
	}

	virtual int OnPreCommand(const std::string &command, const char** parameters, int pcnt, userrec *user, bool validated)
	{
		if ((command == "GLINE") || (command == "KLINE") || (command == "ELINE") || (command == "ZLINE"))
		{
			AddLogEntry(LT_XLINE,user->nick,command[0]+std::string(":")+std::string(parameters[0]),user->server);
		}
		return 0;
	}

	virtual void OnUserConnect(userrec* user)
	{
		AddLogEntry(LT_CONNECT,user->nick,user->host,user->server);
	}

	virtual void OnGlobalConnect(userrec* user)
	{
		AddLogEntry(LT_CONNECT,user->nick,user->host,user->server);
	}

	virtual void OnUserQuit(userrec* user, const std::string &reason)
	{
		AddLogEntry(LT_DISCONNECT,user->nick,user->host,user->server);
	}

	virtual void OnLoadModule(Module* mod, const std::string &name)
	{
		AddLogEntry(LT_LOADMODULE,name,Srv->GetServerName(),Srv->GetServerName());
	}

	virtual ~ModuleSQLLog()
	{
	}
	
	virtual Version GetVersion()
	{
		return Version(1,0,0,1,VF_VENDOR);
	}
	
};

class ModuleSQLLogFactory : public ModuleFactory
{
 public:
	ModuleSQLLogFactory()
	{
	}
	
	~ModuleSQLLogFactory()
	{
	}
	
	virtual Module * CreateModule(Server* Me)
	{
		return new ModuleSQLLog(Me);
	}
	
};


extern "C" void * init_module( void )
{
	return new ModuleSQLLogFactory;
}

