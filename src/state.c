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
#include "str.h"
#include "net.h"

extern main_t me;

// when adding, must check existing lists for existing nicks/servers/channels/etc. to prevent duplicate list entries

client *client_list = NULL;
channel *channel_list = NULL;

client *
client_list_insert(void)
{
	client *c = lcalloc(sizeof(client), 1);
	me.clients++;

	c->next = client_list;
	client_list = c;
	if (c->next)
		c->next->prev = c;
	return c;
}

client *
client_list_search(const char *nick)
{
	client *c = client_list;

	for (; c; c = c->next)
		if (strcasecmp(c->nick, nick) == 0)
			return c;
	return NULL;
}

void
client_list_remove(client *c)
{
	if (c->prev) {
		c->prev->next = c->next;
	} else {
		client_list = c->next;
		if (c->next)
			c->next->prev = NULL;
	}
	if (c->next)
		c->next->prev = c->prev;
	me.clients--;
}

void
client_list_delete(void)
{
	client *t, *c = client_list;

	for (; c; c = t) {
		t = c->next;
		client_delete(c);
	}
	client_list = NULL;
}

int
user_list_search(client **results, int maxsearch, const char *nm, const char *um, const char *hm, const char *gm)
{
	client *c = client_list;
	int r = 0;

	for (; c && r < maxsearch; c = c->next)
		if (IsUser(c) && match(nm, c->nick) && match(um, c->user) && match(hm, c->host) && match(gm, c->name))
		results[r++] = c;

	return r;
}

void
client_list_debug(const char *to)
{
	client *c = client_list;

	for (; c; c = c->next)
		sendhub(":%s NOTICE %s :%s!%s@%s$%s on %s, flags = %x\r\n", me.servername, to, c->nick, c->user, c->host, c->name, c->server->nick, c->flags);
}

client *
user_create(const char *nick, const char *user, const char *host, const char *name, client *server, int flags)
{
	client *c;

	if ((c = client_list_search(nick)))
		client_delete(c);

	c = client_list_insert();

	c->nick = lstrdup(nick);
	c->user = lstrdup(user);
	c->host = lstrdup(host);
	c->name = lstrdup(name);
	c->server = server;
	c->flags = flags;
	c->ts = me.now.tv_sec;

	return c;
}

client *
user_modify(client *c, const char *nick)
{
	client *o;

	// delete ghost/collided/duplicate nick if exists, but first make sure it's not a wiz->WiZ change so we don't delete the client itself
	if ((o = client_list_search(nick)) && c != o)
		client_delete(o);

	c->nick = lrestrdup(c->nick, nick);

	return c;
}

void
client_delete(client *c)
{
	client *s, *t = NULL;
	channel *chan;

	debug(7, "client_delete: %s on %s", c->nick, c->server->nick);
	if (IsServer(c)) {
		me.servers--;
		for (s = client_list; s; s = t) {
			t = s->next;
			if (s->server == c)
				client_delete(s);
		}
	} else {
		for (chan = channel_list; chan; chan = chan->next)
			channel_member_del(chan, c);
	}

	if (c->nick)
		lfree(c->nick, strlen(c->nick) + 1);
	if (c->user)
		lfree(c->user, strlen(c->user) + 1);
	if (c->host)
		lfree(c->host, strlen(c->host) + 1);
	if (c->name)
		lfree(c->name, strlen(c->name) + 1);

	client_list_remove(c);
	lfree(c, sizeof(client));
}

client *
server_create(client *hub, const char *servername, const char *servercomment, int flags)
{
	client *c;

	if ((c = client_list_search(servername)))
		client_delete(c);

	c = client_list_insert();

	c->nick = lstrdup(servername);
	c->name = lstrdup(servercomment);
	if (hub)
		c->server = hub;
	else
		c->server = c;
	c->flags = flags;
	c->ts = me.now.tv_sec;

	me.servers++;
	return c;
}

channel *
channel_list_insert(void)
{
	channel *c = lcalloc(sizeof(channel), 1);

	c->next = channel_list;
	channel_list = c;
	if (c->next)
		c->next->prev = c;

	me.channels++;
	return c;
}

channel *
channel_list_search(const char *name)
{
	channel *c = channel_list;

	for (; c; c = c->next)
		if (strcasecmp(c->name, name) == 0)
			return c;

	return NULL;
}

void
channel_list_remove(channel *c)
{
	if (c->prev) {
		c->prev->next = c->next;
	} else {
		channel_list = c->next;
		if (c->next)
			c->next->prev = NULL;
	}
	if (c->next)
		c->next->prev = c->prev;
	me.channels--;
}

void
channel_list_delete(void)
{
	channel *t, *c = channel_list;

	for (; c; c = t) {
		t = c->next;
		channel_delete(c);
	}
	channel_list = NULL;
}

void
channel_list_debug(const char *to)
{
	channel *c = channel_list;
	cmember *m;

	for (; c; c = c->next) {
		sendhub(":%s NOTICE %s :%s %lu %d members: %s, topic is %s\r\n", me.servername, to, c->name, c->ts, c->membercount, channel_mode_display(c), c->topic);
		for (m = c->members; m; m = m->next)
			sendhub(":%s NOTICE %s :%s has (%d)%s\r\n", me.servername, to, c->name, m->flags, m->user->nick);
	}
}

channel *
channel_create(const char *name, const time_t ts)
// 1204726248.777704 parse: :irc.snuggle.jp SJOIN 1204692615 #OSAnime +stn :@Coolin
{
	channel *c;

	if ((c = channel_list_search(name)))
		channel_delete(c);

	c = channel_list_insert();

	c->name = lstrdup(name);
	c->ts = ts;

	return c;
}

void
channel_delete(channel *c)
{
	debug(3, "channel_delete: deleting %s", c->name);
	while (c->members)
		channel_member_del(c, c->members->user);

	channel_list_remove(c);
	if (c->name)
		lfree(c->name, strlen(c->name) + 1);
	if (c->key)
		lfree(c->key, strlen(c->key) + 1);
	lfree(c, sizeof(channel));
}

cmember *
channel_member_add(channel *chan, client *user, int flags)
{
	cmember *m;

	debug(3, "channel_member_add: adding (%d) %s to %s", flags, user->nick, chan->name);

	if ((m = channel_member_search(chan, user)))
		channel_member_del(chan, user);

	m = lcalloc(sizeof(cmember), 1);

	m->next = chan->members;
	chan->members = m;
	if (m->next)
		m->next->prev = m;

	m->user = user;
	m->flags = flags;

	chan->membercount++;
	user->channelcount++;

	return m;
}

cmember *
channel_member_search(channel *chan, client *user)
{
	cmember *m = chan->members;

	for (; m; m = m->next)
		if (m->user == user)
			return m;
	return NULL;
}

void
channel_member_del(channel *chan, client *user)
{
	cmember *m;

	if (!(m = channel_member_search(chan, user)))
		return;

	debug(3, "channel_member_del: deleting %s from %s", user->nick, chan->name);

	if (m->prev) {
		m->prev->next = m->next;
	} else {
		chan->members = m->next;
		if (m->next)
			m->next->prev = NULL;
	}
	if (m->next)
		m->next->prev = m->prev;

	lfree(m, sizeof(cmember));

	chan->membercount--;
	user->channelcount--;

	if (chan->membercount == 0)
		channel_delete(chan);
}

char *
user_channels_display(client *u, int showall)
{
	static char chanlist[BUFSIZE];
	channel *c;
	cmember *m;

	strlcpy(chanlist, "", BUFSIZE);

	for (c = channel_list; c; c = c->next) {
		if ((m = channel_member_search(c, u)) && (showall || (!IsSetBitMask(c->flags, CMODE_s) && !IsSetBitMask(c->flags, CMODE_p)))) {
			if (IsSetBitMask(m->flags, CMEMBER_OP))
				strlcat(chanlist, "@", BUFSIZE);
			if (IsSetBitMask(m->flags, CMEMBER_HOP))
				strlcat(chanlist, "%", BUFSIZE);
			if (IsSetBitMask(m->flags, CMEMBER_VOICE))
				strlcat(chanlist, "+", BUFSIZE);
			strlcat(chanlist, c->name, BUFSIZE);
			strlcat(chanlist, " ", BUFSIZE);
		}
	}

	return chanlist;
}
