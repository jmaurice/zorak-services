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

#include <sys/types.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <stdio.h>
#include "main.h"
#include "irc.h"
#include "net.h"
#include "state.h"
#include "str.h"
#include "cmsg.h"
#include "replies.h"

extern main_t me;
extern struct _sclient sclients[SCLIENT_TOTAL + 1];

#define	MODE_DEL	-1
#define	MODE_ADD	+1

void
parse_cmode(channel *c, const char **argv)
{
// imnpst - channel modes (no args)
// bkloveI - channel modes (args)
// ohv - cmember modes (args)
	const char *mode;
	client *u;
	cmember *m;
	int y = 0;

	for (mode = *argv; *mode; mode++) {
		switch (*mode) {
			case '-':
				y = MODE_DEL;
				break;
			case '+':
				y = MODE_ADD;
				break;
			case 'i':
			case 'm':
			case 'n':
			case 'p':
			case 's':
			case 'S':
			case 't':
				if (y == MODE_ADD)
					c->flags = SetBitMask(c->flags, ChanModeBitMask(*mode));
				else if (y == MODE_DEL)
					c->flags = ClrBitMask(c->flags, ChanModeBitMask(*mode));
#ifndef NO_SECRET
				if (y == MODE_DEL && strlen(c->name) > 1 && *mode == 's') {
					sendhub(":%s MODE %s +s\r\n", me.servername, c->name);
					c->flags = SetBitMask(c->flags, CMODE_s);
				}
#endif
				break;
			case 'l':
				if (y == MODE_ADD && ++argv) {
					c->memberlimit = atoi(*argv);
					c->flags = SetBitMask(c->flags, ChanModeBitMask(*mode));
				} else if (y == MODE_DEL) {
					c->memberlimit = 0;
					c->flags = ClrBitMask(c->flags, ChanModeBitMask(*mode));
				}
				break;
			case 'k':
				if (y == MODE_ADD && ++argv) {
					if (c->key)
						c->key = lrestrdup(c->key, *argv);
					else
						c->key = lstrdup(*argv);
					c->flags = SetBitMask(c->flags, ChanModeBitMask(*mode));
				} else if (y == MODE_DEL) {
					c->flags = ClrBitMask(c->flags, ChanModeBitMask(*mode));
					if (c->key) {
						lfree(c->key, strlen(c->key) + 1);
						c->key = NULL;
					}
				}
				break;
			case 'o':
			case 'h':
			case 'v':
				if (++argv && (u = client_list_search(*argv)) && (m = channel_member_search(c, u))) {
					if (y == MODE_ADD)
						m->flags = SetBitMask(m->flags, CMPrefixBitMask(*mode));
					else if (y == MODE_DEL)
						m->flags = ClrBitMask(m->flags, CMPrefixBitMask(*mode));
				}
				break;
		}
	}
}

void
parse_umode(client *cptr, const char *mode)
{
	int y = 0;

	while (*mode != '\0')
		switch (*mode++) {
		case '-':
			y = MODE_DEL;
			break;
		case '+':
			y = MODE_ADD;
			break;
		case 'o':
			debug(3, "parse_umode: %s mode %c o", cptr->nick, (y == MODE_ADD ? '+' : '-'));
			(y == MODE_ADD ? SetOper(cptr) : ClrOper(cptr));
			break;
		case 'S':
			debug(3, "parse_umode: %s mode %c S", cptr->nick, (y == MODE_ADD ? '+' : '-'));
			(y == MODE_ADD ? SetService(cptr) : ClrService(cptr));
			break;
		}
}

void
services_intro(int sc)
{
	client *myself = client_list_search(me.servername);
	int i;

	for (i = SCLIENT_MAIN; i <= SCLIENT_TOTAL; i++) {
		sclients[i].cptr = user_create(sclients[i].nick, sclients[i].user, me.servername, sclients[i].name, myself, CLIENT_SERVICE);
		sendhub("NICK %s 1 %lu +oS %s %s %s :%s\r\n", sclients[i].nick, me.now.tv_sec, sclients[i].user, me.servername, me.servername, sclients[i].name);
	}
}

int
services_client(const char *nick)
{
	int i;

	for (i = SCLIENT_MAIN; i <= SCLIENT_TOTAL; i++)
		if (strcasecmp(sclients[i].nick, nick) == 0)
			return i;

	return -1;
}


void
chanserv_join(const char *chan)
{
	channel *c;

	if (!IsChanPrefix(chan[0]))
		return;

	if (!(c = channel_list_search(chan))) {
		c = channel_create(chan, me.now.tv_sec); // XXX change to original registration TS upon joining and set topic
	}

	channel_member_add(c, sclients[SCLIENT_CHAN].cptr, CMEMBER_OP);
	sendhub(":%s SJOIN %lu %s %s :@%s\r\n", me.servername, c->ts, c->name, channel_mode_display(c), sclients[SCLIENT_CHAN].nick);
}

int
m_error(client *cptr, int parc, const char **parv)
{
// ERROR :Closing Link: 127.0.0.1 services.lulz (.)
// XXX - check parc/parv[1] ?
	debug(1, "m_error: %s", parv[1]);
	aborthub("m_error: %s", parv[1]);
	return 1;
}

int
m_pong(client *cptr, int parc, const char **parv)
{
// :irc.snuggle.jp PONG irc.snuggle.jp :services.lulz
	if (me.hubstatus == HUB_NEGOTIATE) {
		logger(LOG_INFO, "received EOB from hub %s", me.hubserver);
		me.hubstatus = HUB_LINKED;
	}
	return 1;
}

int
m_ping(client *cptr, int parc, const char **parv)
{
// PING :irc.snuggle.jp
// :irc.arrogant.org PING irc.arrogant.org :services.lulz
	debug(6, "m_ping: ping? pong!");
	if (cptr)
		sendhub(":%s PONG %s :%s\r\n", me.servername, me.servername, cptr->nick);
	else
		sendhub("PONG :%s\r\n", parv[1]);
	return 1;
}

int
m_trace(client *cptr, int parc, const char **parv)
{
// :wiz TRACE :services.lulz

	debug(3, "m_trace: tracing");
	client_list_debug(parv[0]);
	channel_list_debug(parv[0]);
	return 1;
}

int
m_nick(client *cptr, int parc, const char **parv)
{
// NICK xyst 2 1204700604 +aiow gregp null.arrogant.org irc.arrogant.org :gregp
// :wiz NICK _wiz :1205081545

	client *s;

	if (parc == 3) {
		if (cptr) {
			user_modify(cptr, parv[1]);
			cptr->ts = (time_t)strtol(parv[2], (char **)NULL, 10);
		}
		return 1;
	}

	if (parc < 6)
		return 0;

	if (!(s = client_list_search(parv[7])))
		return 0;

	debug(4, "m_nick: %s!%s@%s$%s on %s", parv[1], parv[5], parv[6], parv[8], s->nick);
	if (!(s = user_create(parv[1], parv[5], parv[6], parv[8], s, CLIENT_USER)))
		return 0;

	parse_umode(s, parv[4]);

	return 1;
}

int
m_quit(client *cptr, int parc, const char **parv)
{
// :Ryu QUIT :Remote host closed the connection

	if (cptr) {
		debug(4, "m_quit: %s!%s@%s", cptr->nick, cptr->user, cptr->host);
		client_delete(cptr);
	}
	return 1;
}

int
m_kill(client *cptr, int parc, const char **parv)
{
// :wiz KILL ChanServ :irc.snuggle.jp!atom.wiz.biz!jmaurice!wiz (hi)
	if (parc > 1 && (cptr = client_list_search(parv[1]))) {
		debug(3, "m_kill: %s!%s@%s", cptr->nick, cptr->user, cptr->host);
		client_delete(cptr);
	}
	return 1;
}

int
m_server(client *cptr, int parc, const char **parv)
{
// SERVER irc.snuggle.jp 1 :welcome to snugglenet
// :irc.snuggle.jp SERVER irc.arrogant.org 2 :lame
	if (parc < 3)
		return 0;
	if (!(cptr = client_list_search((parv[0] ? parv[0] : me.servername)))) {
		logger(LOG_NOTICE, "m_server: parv[1] introduced from parv[0], but parv[0] not in client list?!");
		return 0;
	}

	debug(3, "m_server: %s -> %s (%s)", (parv[0] ? parv[0] : me.servername), parv[1], parv[3]);
	server_create(cptr, parv[1], parv[3], CLIENT_SERVER);
	return 1;
}

int
m_squit(client *cptr, int parc, const char **parv)
{
// SQUIT irc.arrogant.org :irc.snuggle.jp
	if (parc > 1 && (cptr = client_list_search(parv[1]))) {
		debug(3, "m_squit: %s", cptr->nick);
		client_delete(cptr);
	}
	return 1;
}

int
m_sjoin(client *cptr, int parc, const char **parv)
{
// :irc.snuggle.jp SJOIN 1204609345 #BoA* +tn :@ptoor @xyst +Kurot @quoteNinja @kingofbeer @wiz @Coolin @q
// :irc.snuggle.jp SJOIN 1189554381 # +stnk hi :@draxon_ @wiz @aba @xyst @Dysko @q
// :irc.snuggle.jp SJOIN 1189554381 # +stnlk 1337 hi :@draxon_ @wiz @aba @xyst @Dysko @q
	channel *chan;
	cmember *member;
	client *user;
	char nickbuf[BUFSIZE];
	time_t ts;
	char *tmp;
	int flag;

	if (parc < 4 || (ts = strtol(parv[1], (char **)NULL, 10)) < 0)
		return 0;

	strlcpy(nickbuf, parv[parc - 1], BUFSIZE);
	debug(3, "m_sjoin: nickbuf = %s", nickbuf);

	if (!(chan = channel_list_search(parv[2]))) {
		chan = channel_create(parv[2], ts);
		parse_cmode(chan, parv + 3);
#ifndef NO_SECRET
		if (!IsSetBitMask(chan->flags, CMODE_s)) {
			sendhub(":%s MODE %s +s\r\n", me.servername, chan->name);
			chan->flags = SetBitMask(chan->flags, CMODE_s);
		}
#endif
	}

	for (tmp = strtok(nickbuf, " "); tmp; tmp = strtok(NULL, " ")) {
		for (flag = 0; IsChanStatusChar(*tmp); tmp++) {
			if (!*tmp) /* no nick? */
				return 0;
			switch (*tmp) {
				case '@': SetBitMask(flag, CMEMBER_OP); break;
				case '%': SetBitMask(flag, CMEMBER_HOP); break;
				case '+': SetBitMask(flag, CMEMBER_VOICE); break;
			}
		}
		if (!(user = client_list_search(tmp))) {
			logger(LOG_NOTICE, "m_sjoin: join to %s by unknown client %s ?!?!", chan->name, tmp);
			continue;
		}
		debug(3, "m_sjoin: adding (%d) %s to %s", flag, user->nick, chan->name);
		if ((member = channel_member_add(chan, user, flag)) && IsOper(user) && !IsSetBitMask(flag, CMEMBER_OP) && me.hubstatus == HUB_LINKED) {
			debug(3, "m_sjoin: mode +o for oper %s in %s", user->nick, chan->name);
        		member->flags = SetBitMask(member->flags, CMEMBER_OP);
        		sendhub(":%s MODE %s +o %s\r\n", me.servername, chan->name, user->nick);
		}
	}
	return 1;
}

int
m_part(client *cptr, int parc, const char **parv)
{	
// :wiz PART #BoA*
// :Peter PART #pimp :Leaving

	channel *chan;

	if (!cptr || parc < 2 || !(chan = channel_list_search(parv[1])))
		return 0;

	debug(3, "m_part: removing %s from %s", cptr->nick, chan->name);
	channel_member_del(chan, cptr);
	return 1;
}

int
m_kick(client *cptr, int parc, const char **parv)
{
// :wiz KICK # wiz :hi
	channel *chan;
	client *kicked;

	if (!cptr || parc < 3 || !(chan = channel_list_search(parv[1])) || !(kicked = client_list_search(parv[2])))
		return 0;

	debug(3, "m_part: removing %s from %s", kicked->nick, chan->name);
	channel_member_del(chan, kicked);
	return 1;
}

int
m_mode(client *cptr, int parc, const char **parv)
{
// :wiz MODE wiz :+aow
// :wiz MODE #BoA* +o q

	channel *chan;
	client *user;

	if (parc < 3)
		return 0;

	if (IsChanPrefix(parv[1][0])) {
		if (!(chan = channel_list_search(parv[1]))) {
			debug(1, "m_mode: received mode for non-existant target: %s", parv[1]);
			return 0;
		}
		parse_cmode(chan, parv + 2);
		return 1;
	}

	if (!(user = client_list_search(parv[1]))) {
		debug(1, "m_mode: received mode for non-existant target: %s", parv[1]);
		return 0;
	}
	parse_umode(cptr, parv[2]);
	return 1;
}

int
m_message(client *cptr, int parc, const char **parv)
{
// :wiz PRIVMSG # :hi
// :wiz PRIVMSG # :\1ACTION hi\1
// :wiz PRIVMSG ChanServ :\1PING 1204868597 277208\1
// :wiz PRIVMSG ChanServ :hi
// :wiz NOTICE ChanServ :hi
	char *cmd, *args, msg[BUFSIZE];
	struct message *mptr;
	int sc;

	if (!cptr || parc < 3)
		return 0;

	strlcpy(msg, parv[2], BUFSIZE);
	if (!(cmd = strtok(msg, " ")))
		return 0;
	args = strtok(NULL, "");

	if ((sc = services_client(strtok((char *)parv[1], "@"))) == -1)
		return 0;

	if (sc == SCLIENT_OPER && !IsOper(cptr))
		return sreply(sc, cptr->nick, replies[RPL_NOACCESS]);

	for (mptr = msgtab; mptr->cmd; mptr++) {
		if ((mptr->sclient == SCLIENT_ANY || mptr->sclient == sc) && strcasecmp(mptr->cmd, cmd) == 0) {
			if (IsSetBitMask(cptr->flags, mptr->access))
				return (*mptr->func)(sc, cptr, args);
			else
				return sreply(sc, cptr->nick, replies[RPL_NOACCESS]);
		}
	}

	return sreply(sc, cptr->nick, replies[RPL_NOSUCHCMD], sclients[sc].nick);
}

int
m_motd(client *cptr, int parc, const char **parv)
{
// :wiz MOTD :services.lulz

	if (!parv[0])
		return 0;

	sendhub(":%s 375 %s :- %s services!\r\n", me.servername, parv[0], me.servername);
	sendhub(":%s 372 %s :-\r\n", me.servername, cptr->nick);
	sendhub(":%s 372 %s :- lulz\r\n", me.servername, cptr->nick);
	sendhub(":%s 372 %s :-\r\n", me.servername, cptr->nick);
	sendhub(":%s 376 %s :End of MOTD\r\n", me.servername, parv[0]);

	return 1;
}

int
m_admin(client *cptr, int parc, const char **parv)
{
// :wiz ADMIN :services.lulz

	if (!parv[0])
		return 0;

	sendhub(":%s 256 %s :Administrative contact for %s:\r\n", me.servername, parv[0], me.servername);
	sendhub(":%s 257 %s :.\r\n", me.servername, parv[0]);
	sendhub(":%s 258 %s :.\r\n", me.servername, parv[0]);
	sendhub(":%s 259 %s :.\r\n", me.servername, parv[0]);

	return 1;
}

int
m_whois(client *cptr, int parc, const char **parv)
{
// :wiz WHOIS ChanServ :chanserv
// :wiz WHOIS services.lulz :chanserv
	client *who;

	if (!cptr || parc < 3)
		return 0;

	if (!(who = client_list_search(parv[2]))) {
		sendhub(":%s 401 %s %s :No such nick/channel\r\n", me.servername, parv[0], parv[2]);
		return 1;
	}

	sendhub(":%s 311 %s %s %s %s * :%s\r\n", me.servername, parv[0], who->nick, who->user, who->host, who->name);
	if (!IsService(who) && who->channelcount > 0)
		sendhub(":%s 319 %s %s :%s\r\n", me.servername, parv[0], who->nick, user_channels_display(who, IsOper(cptr)));
	if (me.serverhiding)
		sendhub(":%s 312 %s %s %s :%s\r\n", me.servername, parv[0], who->nick, (IsOper(cptr) ? who->server->nick : me.servername), (IsOper(cptr) ? who->server->name : me.serverdesc));
	else
		sendhub(":%s 312 %s %s %s :%s\r\n", me.servername, parv[0], who->nick, who->server->nick, who->server->name);
	if (IsService(who))
		sendhub(":%s 313 %s %s :is a network service\r\n", me.servername, parv[0], who->nick);
	else if (IsOper(who))
		sendhub(":%s 313 %s %s :is an IRC Operator\r\n", me.servername, parv[0], who->nick);
	if (IsMe(who->server))
		sendhub(":%s 317 %s %s %lu %lu :seconds idle, signon time\r\n", me.servername, parv[0], who->nick, me.now.tv_sec - who->ts, who->ts);
	sendhub(":%s 318 %s %s :End of /WHOIS list.\r\n", me.servername, parv[0], who->nick);

	return 1;
}

int
m_stats(client *cptr, int parc, const char **parv)
{
// :wiz STATS l :services.lulz
	if (!cptr || parc < 3)
		return 0;

	sendhub(":%s 481 %s :Access Denied!\r\n");
	return 1;
}

int
m_version(client *cptr, int parc, const char **parv)
{
// :wiz VERSION :services.lulz
// :irc.arrogant.org 351 ChanServ ircd-ratbox-2.2.6(20070514_2-23959). irc.arrogant.org :egGHIKMpZ TS6ow 007
// :irc.arrogant.org 105 ChanServ CHANTYPES=&# EXCEPTS INVEX CHANMODES=eIb,k,l,imnpst CHANLIMIT=&#:15 PREFIX=(ov)@+ MAXLIST=beI:25 NETWORK=snugglenet MODES=4 STATUSMSG=@+ KNOCK CALLERID=g :are supported by this server
	if (!parv[0])
		return 0;

	sendhub(":%s 351 %s services :%s TS5\r\n", me.servername, parv[0], me.servername);

	return 1;
}

int
m_connect(client *cptr, int parc, const char **parv)
{
// :wiz CONNECT hax.hax 1 :services.lulz
	return 1;
}

int
m_wallops(client *cptr, int parc, const char **parv)
{
// :irc.arrogant.org WALLOPS :Remote CONNECT nurr.nurr.nurr 6660 from wiz
	return 1;
}

int
m_operwall(client *cptr, int parc, const char **parv)
{
// :wiz OPERWALL :hi
	return 1;
}

int
m_time(client *cptr, int parc, const char **parv)
{
// :wiz TIME :services.lulz
// :irc.arrogant.org 391 ChanServ irc.arrogant.org :Sunday March 9 2008 -- 08:00:43 -04:00
	if (!parv[0])
		return 0;

	sendhub(":%s 391 %s %s :%s\r\n", me.servername, parv[0], me.servername, lctime(me.now.tv_sec));
	return 1;
}

int
m_encap(client *cptr, int parc, const char **parv)
{
// :wiz^^v ENCAP * LOGIN wiz
	if (!cptr || parc < 4)
		return 0;

	if (strcmp(parv[2], "LOGIN") == 0) {
		debug(3, "m_encap: logging in %s as %s", cptr->nick, parv[3]);
	}
	return 1;
}

// :wiz LUSERS * :services.lulz
// ENCAP irc.snuggle.jp RSFNC wiz wiz2 1205163916(60 seconds ago) 1205081547(wiz's ts)
// :wiz ENCAP services.lulz KLINE 0 test test.test :test
// :wiz ENCAP * KLINE 0 test test.test :test
// :wiz ENCAP * UNKLINE test test.test
