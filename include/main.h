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

#ifndef __MAIN_H_
#define __MAIN_H_
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "state.h"

#define BUFSIZE 16384

typedef struct main_t {
	struct timeval		now;
	int			debug;
	struct	in_addr		ip;

	char *			conffile;
	char *			servername;
	char *			serverhost;
	char *			serverdesc;
	int			serverhiding;

	int			hubstatus;
	int			hubsock;
	char *			hubhost;
	char *			hubpasswd;
	int			hubport;
	char *			hubserver;
	time_t			hubwait;
	char			hub_read_buf[BUFSIZE];
	int			hub_read_len;

	u_long			memusage;
	u_long			servers;
	u_long			clients;
	u_long			channels;
	u_long			snuggles;
} main_t;

#define	SCLIENT_ANY	0

#define	SCLIENT_MAIN	1
#define	SCLIENT_OPER	2
#define	SCLIENT_NICK	3
#define	SCLIENT_CHAN	4
#define	SCLIENT_MAIL	5

#define	SCLIENT_TOTAL	5

struct _sclient {
	char *nick;
	char *user;
	char *name;
	client *cptr;
};

#define	HUB_WAIT		-1
#define	HUB_INITIAL		0
#define	HUB_NOTCONN		1000
#define	HUB_TRYDNS		2000
#define	HUB_GOTDNS		3000
#define	HUB_TRYCONN		4000
#define	HUB_GOTCONN		5000
#define	HUB_NEGOTIATE		6000
#define	HUB_LINKED		10000

void				logger			(int, char *, ...);
void				debug			(int, char *, ...);
int				parse_conf		(void);

#endif
