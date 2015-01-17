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
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <err.h>
#include "main.h"
#include "net.h"
#include "str.h"
#include "irc.h"
#include "state.h"

extern main_t me;
sock *sock_list = NULL;
extern struct _sclient sclients[SCLIENT_TOTAL + 1];

sock *
sock_add(int sok)
{
	sock *s;

	if ((s = calloc(1, sizeof(sock))) == NULL) {
		logger(LOG_CRIT, "sock_add(%d): calloc(%d) failed!", sok, sizeof(sock));
		return NULL;
	}

	if (sock_list != NULL) {
		s->next = sock_list;
		sock_list->prev = s;
	}

	s->sock_fd = sok;
	return s;
}

void
sock_del(sock *s)
{
	if (sock_list == s)
		sock_list = (s->next ? s->next : NULL);
	if (s->prev != NULL)
		s->prev->next = s->next;
	if (s->next != NULL)
		s->next->prev = s->prev;

	free(s);
}

int
sock_set_nonblock(int sock)
{
	 struct linger linger_val = { 1, 1 };
	 int flags;

	 if ((flags = fcntl(sock, F_GETFL, 0)) == -1 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
		return -1;
	 if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *)&linger_val, sizeof(linger_val)) == -1)
		return -1;

	 return 0;
}

int
sock_get_error(int sock)
{
	 int errv, errlen = sizeof(errv);

	 if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &errv, &errlen) < 0) {
		logger(LOG_CRIT, "getsockopt(SO_ERROR) failed on socket(%d) %d: %s", sock, errv, strerror(errv));
		return -1;
	 }

	 return errv;
}

void
sendhub(char *fmt, ...)
{
	char msgbuf[BUFSIZE];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(msgbuf, BUFSIZE, fmt, ap);
	if (write(me.hubsock, msgbuf, strlen(msgbuf)) == -1) {
		logger(LOG_NOTICE, "error writing to hub %s: %s", me.hubserver, strerror(errno));
		return;
	}
	if (strlen(msgbuf) > 2) {
		if (msgbuf[strlen(msgbuf)] == '\n')
			msgbuf[strlen(msgbuf) - 1] = 0;
		if (msgbuf[strlen(msgbuf)] == '\r')
			msgbuf[strlen(msgbuf) - 1] = 0;
	}
	debug(2, "sendhub>: %s", msgbuf);
	va_end(ap);
}

int
sreply(int sc, const char *to, char *fmt, ...)
{
	char msgbuf[BUFSIZE];
	va_list ap;

	if (sc < SCLIENT_MAIN || sc > SCLIENT_TOTAL)
		return 0;

	va_start(ap, fmt);
	vsnprintf(msgbuf, BUFSIZE, fmt, ap);
	sendhub(":%s NOTICE %s :%s\r\n", sclients[sc].nick, to, msgbuf);
	va_end(ap);
	return 1;
}


void
aborthub(char *fmt, ...)
{
	char reason[BUFSIZE];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(reason, BUFSIZE, fmt, ap);
	logger(LOG_NOTICE, "disconnecting from hub %s: %s", me.hubserver, reason);
	va_end(ap);

	close(me.hubsock);

	client_list_delete();
	channel_list_delete();

	me.hubstatus = HUB_WAIT;
	me.hubwait = me.now.tv_sec + 30;
	logger(LOG_INFO, "reconnecting to hub in 30 seconds");
}


void
io_loop()
{
	fd_set readfds, writefds, exceptfds;
	struct sockaddr_in sin;
	// size_t ssin = sizeof(sin); accept()
	int errv, r; //, fdsetsize = FD_SETSIZE; adns()
	u_short checkfds = 0;
	struct timeval tv;
	struct stat sb;
	sock *s, *t;
	char buf[BUFSIZE];
	int i, d, x;

loop:
	// zero out fd sets
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

	// get current time from kernel
	gettimeofday(&me.now, NULL);

	// timers

	switch (me.hubstatus) {
		case HUB_WAIT:
			if (me.now.tv_sec > me.hubwait) {
				me.hubwait = 0;
				me.hubstatus = HUB_INITIAL;
			}
			break;
		case HUB_INITIAL:
			memset(&me.hub_read_buf, 0, BUFSIZE);
			me.hub_read_len = 0;
			if ((me.hubsock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
				logger(LOG_CRIT, "io_loop: cannot allocate hub socket!");
				me.hubstatus = HUB_WAIT;
				me.hubwait = me.now.tv_sec + 30;
				break;
			}
			if (sock_set_nonblock(me.hubsock) == -1) {
				logger(LOG_CRIT, "io_loop: cannot set hub socket non-blocking!");
				me.hubstatus = HUB_WAIT;
				me.hubwait = me.now.tv_sec + 30;
				break;
			}
			me.hubstatus = HUB_NOTCONN;
			break;
		case HUB_NOTCONN:
			logger(LOG_NOTICE, "connecting to hub %s:%d", me.hubserver, me.hubport);
			memset(&sin, 0, sizeof(sin));
			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = inet_addr(me.hubhost);
			sin.sin_port = htons(me.hubport);
			if (connect(me.hubsock, (struct sockaddr *)&sin, sizeof(sin)) == -1 && errno != EINPROGRESS) {
				logger(LOG_NOTICE, "io_loop: connect to hub %s failed: %s", me.hubserver, strerror(errno));
				me.hubstatus = HUB_WAIT;
				me.hubwait = me.now.tv_sec + 30;
				break;
			}
			me.hubstatus = HUB_TRYCONN;
			break;
		case HUB_TRYCONN:
			FD_SET(me.hubsock, &writefds);
			break;
		case HUB_GOTCONN:
			logger(LOG_NOTICE, "connected to hub %s", me.hubserver);
			sendhub("CAPAB :CAP ENCAP QS KLN GLN UNKLN KNOCK TB UNKLN CLUSTER SERVICES RSFNC HUB\r\n"); //add SAVE later
			sendhub("PASS %s :TS\r\n", me.hubpasswd);
			sendhub("SERVER %s 0 :%s\r\n", me.servername, me.serverdesc);
			sendhub("SVINFO 5 5 0 :%lu\r\n", time(NULL));
			server_create(NULL, me.servername, me.serverdesc, CLIENT_ME);
			services_intro(0);
			sendhub("PING :%s\r\n", me.servername);
			me.hubstatus = HUB_NEGOTIATE;
			FD_SET(me.hubsock, &readfds);
			break;
		case HUB_NEGOTIATE:
		case HUB_LINKED:
			FD_SET(me.hubsock, &readfds);
			break;
	}

	// walk sock list
	for (s = sock_list; s;) {
		if (checkfds == 1) {
			logger(LOG_NOTICE, "io_loop: checking fds");
			if (fstat(s->sock_fd, &sb) == -1) {
				logger(LOG_NOTICE, "io_loop: sock %d had closed fd", s->sock_fd);
				goto delsock;
			}
		}
		if (s->sock_flags & SOCK_DEL) {
		delsock:
			close(s->sock_fd);
			t = s->next;
			sock_del(s);
			s = t;
			continue;
		}
		if (me.now.tv_sec - s->sock_last > 300) { /* stale socket */
			logger(LOG_NOTICE, "disconnecting idle socket");
			goto delsock;
		}
	}

	// select() to timeout after 1 second
	tv = (struct timeval){ 1, 0 };

	debug(6, "io_loop: before select()");
	// before select
	r = select(FD_SETSIZE, &readfds, &writefds, &exceptfds, &tv);
	debug(6, "io_loop: after select()");
	// after select

	switch (r) {
		case -1:
			if (errno == EBADF)
				checkfds = 1;
			else if (errno != EINTR)
				logger(LOG_CRIT, "io_loop: select() returned error %d: %s", errno, strerror(errno));
				// del all socks?
			goto loop;
		case 0:
			/* timeout hit, no socks ready */
			goto loop;
	}

	if (me.hubstatus == HUB_TRYCONN && FD_ISSET(me.hubsock, &writefds)) {
		if ((errv = sock_get_error(me.hubsock)) != 0) {
			logger(LOG_NOTICE, "connection to hub %s failed: %s", me.hubserver, strerror(errv));
			me.hubstatus = HUB_WAIT;
			me.hubwait = me.now.tv_sec + 30;
			goto loop;
		}
		me.hubstatus = HUB_GOTCONN;
	} else if (me.hubstatus >= HUB_NEGOTIATE && FD_ISSET(me.hubsock, &readfds)) {
		if (me.hub_read_len >= BUFSIZE - 3) {
			aborthub("recvq exceeded");
			goto loop;
		}
		switch ((r = read(me.hubsock, &me.hub_read_buf[me.hub_read_len], BUFSIZE - me.hub_read_len))) {
			case -1: /* error */
				aborthub("read error: %s", strerror(errno));
				goto loop;
			case 0: /* EOF */
				aborthub("remote host closed the connection");
				goto loop;
			default:
				debug(6, "io_loop: read %d bytes from hub", r);
				me.hub_read_len += r;
		}
		for (i = d = x = 0; i+1 < BUFSIZE && i+1 <= me.hub_read_len; i++) {
			if (me.hub_read_buf[i] == '\r' && me.hub_read_buf[i+1] == '\n') {
				me.hub_read_buf[i] = 0;
				strlcpy(buf, &me.hub_read_buf[d], BUFSIZE);
				parsehub(buf);
				d = i + 2;
				i++;
				x++;
			}
		}

		if (x > 0) {
			memset(buf, 0, BUFSIZE);
			memcpy(buf, &me.hub_read_buf[d], (i - d));
			memcpy(&me.hub_read_buf, buf, BUFSIZE);
			me.hub_read_len = (i - d);
		}
	}
	goto loop;

}
