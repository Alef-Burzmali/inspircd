/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd: (C) 2002-2009 InspIRCd Development Team
 * See: http://wiki.inspircd.org/Credits
 *
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#include "inspircd.h"
#include "mode.h"
#include "channels.h"
#include "users.h"
#include "modes/cmode_t.h"

ModeChannelTopicOps::ModeChannelTopicOps() : ModeHandler(NULL, 't', PARAM_NONE, MODETYPE_CHANNEL)
{
}

ModeAction ModeChannelTopicOps::OnModeChange(User*, User*, Channel* channel, std::string&, bool adding)
{
	if (channel->modes[CM_TOPICLOCK] != adding)
	{
		channel->modes[CM_TOPICLOCK] = adding;
		return MODEACTION_ALLOW;
	}
	else
	{
		return MODEACTION_DENY;
	}
}

