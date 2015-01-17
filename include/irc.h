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

#ifndef __IRC_H__
#define __IRC_H__
#include "state.h"

void		services_intro		(int);
int		services_client		(const char *);
void		chanserv_join		(const char *);
void		parse_cmode		(channel *, const char **);
void		parse_umode		(client *, const char *);

int		m_error			(client *, int, const char **);
int		m_pong			(client *, int, const char **);
int		m_ping			(client *, int, const char **);
int		m_trace			(client *, int, const char **);
int		m_nick			(client *, int, const char **);
int		m_quit			(client *, int, const char **);
int		m_kill			(client *, int, const char **);
int		m_server		(client *, int, const char **);
int		m_squit			(client *, int, const char **);
int		m_sjoin			(client *, int, const char **);
int		m_part			(client *, int, const char **);
int		m_kick			(client *, int, const char **);
int		m_mode			(client *, int, const char **);
int		m_message		(client *, int, const char **);
int		m_motd			(client *, int, const char **);
int		m_admin			(client *, int, const char **);
int		m_whois			(client *, int, const char **);
int		m_stats			(client *, int, const char **);
int		m_version		(client *, int, const char **);
int		m_connect		(client *, int, const char **);
int		m_wallops		(client *, int, const char **);
int		m_operwall		(client *, int, const char **);
int		m_time			(client *, int, const char **);
int		m_encap			(client *, int, const char **);


#endif /* __IRC_H_ */
