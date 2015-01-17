/*
 * zorak services - a framework for implementing irc network service bots
 * Copyright 2002-2015 J. Maurice <j@wiz.biz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "main.h"
#include "state.h"
#include "services.h"
#include "str.h"
#include "net.h"

extern main_t me;
extern client *client_list;
extern struct _sclient sclients[SCLIENT_TOTAL + 1];

int
sc_help(int sc, client *cptr, char *args)
{
#if 0
	struct message *mptr;

	for (mptr = msgtab; mptr->cmd; mptr++)
		if ((mptr->sclient == sc) && IsSetBitMask(cptr->flags, mptr->access))
#endif
	sreply(sc, cptr->nick, "Available commands:");
	sreply(sc, cptr->nick, " ");
	switch (sc) {
		case SCLIENT_OPER:
		if (IsLeet(cptr)) {
			sreply(sc, cptr->nick, "\2JUPE\2       Remove a compromised server from the network");
			sreply(sc, cptr->nick, " ");
		}
			sreply(sc, cptr->nick, "\2STATUS\2     Status of the network and services");
			sreply(sc, cptr->nick, "\2OPME\2       Gain operator status in a channel");
			sreply(sc, cptr->nick, "\2GLINE\2      Add a tkline to all client servers");
			sreply(sc, cptr->nick, "\2UNGLINE\2    Remove a previously added GLINE");
			sreply(sc, cptr->nick, "\2LONERS\2     List clients on the network in 0 channels");
			sreply(sc, cptr->nick, "\2CLONES\2     List clients connecting from the same host");
			break;
		default:
			sreply(sc, cptr->nick, "(no commands available at this time)");
			break;
	}
	sreply(sc, cptr->nick, " ");
	sreply(sc, cptr->nick, "All commands sent to %s are logged. Have a nice day!", sclients[sc].nick);
	return 1;
}

int
os_raw(int sc, client *cptr, char *args)
{
	if (args && strlen(args) > 1)
		sendhub("%s\r\n", args);
	return 1;
}

int
os_opme(int sc, client *cptr, char *args)
{
	channel *chan;
	cmember *member;

	if (!args || strlen(args) < 1)
		return sreply(sc, cptr->nick, "Usage: /msg %s OPME channel", sclients[sc].nick);
	if (!(chan = channel_list_search(args)))
		return sreply(sc, cptr->nick, "Channel %s does not exist.", args);
	if (!(member = channel_member_search(chan, cptr)))
		return sreply(sc, cptr->nick, "You are not in %s", args);

	member->flags = SetBitMask(member->flags, CMEMBER_OP);
	sendhub(":%s MODE %s +o %s\r\n", me.servername, chan->name, cptr->nick);
	return 1;
}

int
os_gline(int sc, client *cptr, char *args)
{
	char *um, *hm, *reason;

	if (!args || strlen(args) < 3 || !(um = strtok(args, "@")) || !(hm = strtok(NULL, " ")) || !(reason = strtok(NULL, "")))
		return sreply(sc, cptr->nick, "Usage: /msg %s GLINE *@*.host.mask reason", sclients[sc].nick);

	sendhub(":%s ENCAP * KLINE 864000 %s %s :%s\r\n", sclients[SCLIENT_MAIN].nick, um, hm, reason);
	return 1;
}

int
os_ungline(int sc, client *cptr, char *args)
{
	char *um, *hm;

	if (!args || strlen(args) < 3 || !(um = strtok(args, "@")) || !(hm = strtok(NULL, " ")))
		return sreply(sc, cptr->nick, "Usage: /msg %s UNGLINE *@*.host.mask", sclients[sc].nick);

	sendhub(":%s ENCAP * UNKLINE %s %s\r\n", sclients[SCLIENT_MAIN].nick, um, hm);
	return 1;
}

int
os_status(int sc, client *cptr, char *args)
{
	sreply(sc, cptr->nick, "Network Status:");
	sreply(sc, cptr->nick, " ");
	sreply(sc, cptr->nick, "Clients: %lu", me.clients);
	sreply(sc, cptr->nick, "Servers: %lu", me.servers);
	sreply(sc, cptr->nick, "Channels: %lu", me.channels);
	sreply(sc, cptr->nick, "Snuggles: %lu", me.snuggles);
	sreply(sc, cptr->nick, "Memory Usage: %lu", me.memusage);
	sreply(sc, cptr->nick, " ");

	return 1;
}

int
os_loners(int sc, client *cptr, char *args)
{
	client *c = client_list;
	int i = 0;

	sreply(sc, cptr->nick, "The following users are not in any channels:");

	for (; c; c = c->next)
		if (IsUser(c) && c->channelcount == 0)
			sreply(sc, cptr->nick, "%d) [%s] %s!%s@%s$%s", ++i, c->server->nick, c->nick, c->user, c->host, c->name);

	sreply(sc, cptr->nick, "Total of %d loners on the network.", i);
	return 1;
}

int
os_who(int sc, client *cptr, char *args)
{
#define MAXSEARCH 200
	client *results[MAXSEARCH];
	char *nm, *um, *hm, *gm;
	int i, r;

	if (!args || !(nm = strtok(args, "!")) || !(um = strtok(NULL, "@")) || !(hm = strtok(NULL, "$")) || !(gm = strtok(NULL, "")))
		return sreply(sc, cptr->nick, "Usage: /msg %s WHO mask!mask@mask$mask", sclients[sc].nick);

	if ((r = user_list_search(results, MAXSEARCH, nm, um, hm, gm)) == 0)
		return sreply(sc, cptr->nick, "No users found: %s!%s@%s$%s", nm, um, hm, gm);

	for (i = 0; i < r; i++) {
		sreply(sc, cptr->nick, "%d) [%s] %s!%s@%s$%s (%d channels)", i + 1, results[i]->server->nick, results[i]->nick, results[i]->user, results[i]->host, results[i]->name, results[i]->channelcount);
	}

	return sreply(sc, cptr->nick, "A total of %d users matched your search (%s!%s@%s$%s).", r, nm, um, hm, gm);
}

#if 0
int
os_jupe(int sc, client *cptr, char *args)
{
	client *sptr, *myself = client_list_search(me.servername);
	char *reason = "(no reason)";

	if (!args || strlen(args) < 3)
		return sreply(sc, cptr->nick, "Usage: /msg %s JUPE servername [reason]");
	if ((sptr = client_list_search(args)) && IsServer(sptr))
		sendhub(":%s SQUIT %s :%s\r\n", me.servername, server, (reason ? reason : "JUPED"));
	server_create(myself, parv[1], (reason ? reason : "JUPED"), CLIENT_JUPED)
	sendhub(":%s OPERWALL :%s has \2juped\2 %s! (%s)\r\n", me.servername, args, (reason ? reason : "no reason given"));
	sendhub(":%s SERVER %s 1 :%s\r\n", me.servername, name, reason);
}
#endif
