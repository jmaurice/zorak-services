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

#ifndef __STATE_H_
#define __STATE_H_
#include <time.h>

typedef struct _client {
	struct _client *next;
	struct _client *prev;
	struct _client *server;
	char *nick;
	char *user;
	char *host;
	char *name;
	time_t ts;
	int flags;
	int channelcount;
} client;

client *	client_list_insert	(void);
client *	client_list_search	(const char *);
void		client_list_remove	(client *);
void		client_list_delete	(void);
void		client_list_debug	(const char *);
void		client_delete		(client *);
client *	user_create		(const char *, const char *, const char *, const char *, client *, int);
client *	user_modify		(client *, const char *);
client *	server_create		(client *, const char *, const char *, int);
int		user_list_search	(client **, int, const char *, const char *, const char *, const char *);

#define	CLIENT_ME	0x00000001
#define	IsMe(x)		(x->flags & CLIENT_ME)
#define	SetMe(x)	(x->flags |= CLIENT_ME)
#define	CLIENT_SERVICE	0x00000002
#define	IsService(x)	(x->flags & CLIENT_SERVICE)
#define	SetService(x)	(x->flags |= CLIENT_SERVICE)
#define	ClrService(x)	(x->flags &= ~CLIENT_SERVICE)
#define	CLIENT_SERVER	0x00000004
#define	IsServer(x)	(x->flags & CLIENT_SERVER)
#define	SetServer(x)	(x->flags |= CLIENT_SERVER)
#define	CLIENT_JUPED	0x00000008
#define	IsJuped(x)	(x->flags & CLIENT_JUPED)
#define	SetJuped(x)	(x->flags |= CLIENT_JUPED)
#define	CLIENT_USER	0x00000010
#define	IsUser(x)	(x->flags & CLIENT_USER)
#define	SetUser(x)	(x->flags |= CLIENT_USER)
#define	CLIENT_OPER	0x00000020
#define	IsOper(x)	(x->flags & CLIENT_OPER)
#define	SetOper(x)	(x->flags |= CLIENT_OPER)
#define	ClrOper(x)	(x->flags &= ~CLIENT_OPER)
#define	CLIENT_LEET	0x00000040
#define	IsLeet(x)	(x->flags & CLIENT_LEET)
#define	SetLeet(x)	(x->flags |= CLIENT_LEET)
#define	ClrLeet(x)	(x->flags &= ~CLIENT_LEET)

typedef struct _cmember {
	struct _cmember *next;
	struct _cmember *prev;
	client *user;
	int flags;
} cmember;

typedef struct _channel {
	struct _channel *next;
	struct _channel *prev;
	char *name;
	time_t ts;
	cmember *members;
	int flags;
	char *topic;
	char *key;
	int memberlimit;
	int membercount;
} channel;

channel *	channel_list_insert	(void);
channel *	channel_list_search	(const char *);
void		channel_list_remove	(channel *);
void		channel_list_delete	(void);
void		channel_list_debug	(const char *);
channel *	channel_create		(const char *, const time_t);
void		channel_delete		(channel *);
cmember *	channel_member_add	(channel *, client *, int);
cmember *	channel_member_search	(channel *, client *);
void		channel_member_del	(channel *, client *);
char *		user_channels_display	(client *, int);

// bhiklmnopstveI - all possible modes
// imnpst - channel modes (no args)
// bkloveI - channel modes (args)
// ohv - cmember modes (args)

#define	IsSetBitMask(i, m)	((i & m) == (m))
#define	SetBitMask(i, m)	((i) |= (m))
#define	ClrBitMask(i, m)	((i) &= ~(m))

#define	ChanModeBitMask(x)	(x == 'i' ? CMODE_i : x == 'm' ? CMODE_m : x == 'n' ? CMODE_n : x == 'p' ? CMODE_p : x == 's' ? CMODE_s : x == 't' ? CMODE_t : x == 'k' ? CMODE_k : x == 'l' ? CMODE_l : x == 'S' ? CMODE_S : 0)

#define	CMODE_i		0x00000001
#define	CMODE_m		0x00000002
#define	CMODE_n		0x00000004
#define	CMODE_p		0x00000008
#define	CMODE_s		0x00000010
#define	CMODE_S		0x00000020
#define	CMODE_t		0x00000040
#define	CMODE_k		0x00000080
#define	CMODE_l		0x00000100

#define	CMPrefixBitMask(x)	(x == 'o' ? CMEMBER_OP : x == 'h' ? CMEMBER_HOP : x == 'v' ? CMEMBER_VOICE : CMEMBER_NOOB)
#define	CMStatusBitMask(x)	(x == '@' ? CMEMBER_OP : x == '%' ? CMEMBER_HOP : x == '+' ? CMEMBER_VOICE : CMEMBER_NOOB)
#define	CMEMBER_NOOB	0x00000000
#define	CMEMBER_OP	0x00000001
#define	CMEMBER_HOP	0x00000002
#define	CMEMBER_VOICE	0x00000004

#endif
