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

#ifndef __NET_H__
#define __NET_H__
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>

#define	SOCK_DEL	0x0001
#define	SOCK_TRY	0x0002
#define	SOCK_ESTAB	0x0004

#define	SOCK_DNS	0x0100
#define	SOCK_HUB	0x0200
#define	SOCK_DCC	0x0400

typedef struct sock_t {
	struct sock_t *			prev;
	struct sock_t *			next;
	int				sock_fd;
	int				sock_flags;
	time_t				sock_last;

	char				sock_buf[BUFSIZE];
	size_t				sock_buf_len;
	char *				sock_buf_ptr;
} sock;

void		io_loop			(void);
sock *		sock_add		(int);
void		sock_del		(sock *);
int		sock_set_nonblock	(int sock);
int		sock_get_error		(int);
void		sendhub			(char *, ...);
void		aborthub		(char *, ...);
int		sreply			(int, const char *, char *, ...);

#endif /* __NET_H_ **/
