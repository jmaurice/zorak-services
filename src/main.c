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
#define MAXLFLD 10
#define CONFFILE "services.conf"

main_t me;
struct _sclient sclients[SCLIENT_TOTAL + 1];

void
logger(int priority, char *input, ...)
{
	char logbuf[BUFSIZE];
	va_list ap;

	va_start(ap, input);
	vsnprintf(logbuf, BUFSIZE, input, ap);
	syslog(priority, "%s", logbuf);
	if (me.debug)
		printf("%lu.%06lu *** LOG: %s\n", me.now.tv_sec, me.now.tv_usec, logbuf);
	va_end(ap);
}

void
debug(int debug, char *input, ...)
{
	char logbuf[BUFSIZE];
	va_list ap;

	if (me.debug < debug)
		return;

	va_start(ap, input);
	vsnprintf(logbuf, BUFSIZE, input, ap);
	if (0)
		syslog(LOG_DEBUG, "%s", logbuf);
	printf("%lu.%06lu %s%s", me.now.tv_sec, me.now.tv_usec, logbuf, (strchr(logbuf, '\n') ? "" : "\n"));
	fflush(stdout);
	va_end(ap);
}

int
parse_conf(void)
{
	char line[BUFSIZE], *field[MAXLFLD];
	FILE *fd = NULL;
	int i, ln, x;

	if ((fd = fopen(me.conffile, "r")) == NULL)
		return 1;

/* read each line of configuration file and parse it out */
	for (ln = 0; fgets(line, BUFSIZE, fd); ln++) {
		if (strlen(line) < 2 || line[0] == '#' || !strtok(line, ":"))
			continue;
		for(i = 0; i < MAXLFLD && (field[i] = strtok(NULL, ":")); i++)
			/* do nothing */;
		if (i < 1)
			continue;
		field[i - 1][strlen(field[i - 1]) - 1] = 0;
		switch (line[0]) {
			case '#':
				// comment
				break;
			case 'C': /* C:127.0.0.1:momotaro:irc.wiz.biz:6660 */
				if (i < 4) {
					logger(LOG_NOTICE, "parse_conf: ignoring line %d: too few arguments", ln);
					break;
				}
				if (atoi(field[3]) == 0 || atoi(field[3]) > 65535) {
					logger(LOG_NOTICE, "parse_conf: ignoring line %d: invalid port %s", ln, field[3]);
					break;
				}

				if (me.hubhost != NULL)
					lfree(me.hubhost, strlen(me.hubhost) + 1);
				me.hubhost = lstrdup(field[0]);
				if (me.hubpasswd != NULL)
					lfree(me.hubpasswd, strlen(me.hubpasswd) + 1);
				me.hubpasswd = lstrdup(field[1]);
				if (me.hubserver != NULL)
					lfree(me.hubserver, strlen(me.hubserver) + 1);
				me.hubserver = lstrdup(field[2]);
				me.hubport = atoi(field[3]);

				break;
			case 'M': /* M:services2.newnet.net:10.7.9.2:NewNet Operator Services */
				if (i < 3) {
					logger(LOG_NOTICE, "parse_conf: ignoring line %d: too few arguments", ln);
					break;
				}
				if (me.servername == NULL) /* must restart to change server name */
					me.servername = lstrdup(field[0]);

				if (me.serverhost != NULL)
					lfree(me.serverhost, strlen(me.serverhost) + 1);
				me.serverhost = lstrdup(field[1]);
				me.ip.s_addr = inet_addr(field[1]);

				if (me.serverdesc != NULL)
					lfree(me.serverdesc, strlen(me.serverdesc) + 1);
				me.serverdesc = lstrdup(field[2]);

				break;
			case 'N': // N:1:SERVICES:services:snugglecake services development
				if (i < 3) {
					logger(LOG_NOTICE, "parse_config: ignoring line %d: too few arguments", ln);
					break;
				}
				if ((x = atoi(field[0])) < SCLIENT_MAIN || x > SCLIENT_TOTAL) {
					logger(LOG_NOTICE, "parse_config: ignoring line %d: invalid sclient number (%d)", ln, x);
					break;
				}
				if (sclients[x].nick)
					lfree(sclients[x].nick, strlen(sclients[x].nick) + 1);
				sclients[x].nick = lstrdup(field[1]);
				if (sclients[x].user)
					lfree(sclients[x].user, strlen(sclients[x].user) + 1);
				sclients[x].user = lstrdup(field[2]);
				if (sclients[x].name)
					lfree(sclients[x].name, strlen(sclients[x].name) + 1);
				sclients[x].name = lstrdup(field[3]);
				break;
		}
	}
	// add conf checking !! what if no c line? lulz

	return 0;
}

int
main(int argc, char **argv)
{
	int i;

/* initialize myself */
	memset(&me, 0, sizeof(me));
	me.conffile = CONFFILE;
	gettimeofday(&me.now, NULL);

/* ignore sigpipe */
	signal(SIGPIPE, SIG_IGN);

/* parse command line options */
	while ((i = getopt(argc, argv, "df:")) != -1) {
		switch (i) {
			case 'd':
				me.debug++;
				break;
			case 'f':
				me.conffile = lstrdup(optarg);
				break;
		}
	}

/* parse initial configuration */
	if (parse_conf() != 0)
		err(1, "Unable to open %s", me.conffile);

/* fork if not in debug mode */
	if (me.debug) {
		logger(LOG_INFO, "running in the foreground (pid %d)", getpid());
		logger(LOG_NOTICE, "running in debug mode (level %d)", me.debug);
	} else {
		if (fork() != 0)
			return 0;
		logger(LOG_NOTICE, "running in the background (pid %d)", getpid());
	}

/* infinite loop */
	io_loop();

/* never reached, but needed for compilier */
	return 0;
}
