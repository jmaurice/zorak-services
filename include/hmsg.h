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

#ifndef __HMSG_H_
#define __HMSG_H_

static struct command {
	char *cmd;
	int (*func)();
} cmdtab[] = {
	{ "ERROR",	m_error		},
	{ "PING",	m_ping		},
	{ "PONG",	m_pong		},
	{ "NICK",	m_nick		},
	{ "SERVER",	m_server	},
	{ "QUIT",	m_quit		},
	{ "KILL",	m_kill		},
	{ "SQUIT",	m_squit		},
	{ "SJOIN",	m_sjoin		},
	{ "PART",	m_part		},
	{ "KICK",	m_kick		},
	{ "MODE",	m_mode		},
	{ "PRIVMSG",	m_message	},
	{ "NOTICE",	m_message	},
	{ "TRACE",	m_trace		},
	{ "WHOIS",	m_whois		},
	{ "STATS",	m_stats		},
	{ "ADMIN",	m_admin		},
	{ "MOTD",	m_motd		},
	{ "VERSION",	m_version	},
	{ "CONNECT",	m_connect	},
	{ "WALLOPS",	m_wallops	},
	{ "OPERWALL",	m_operwall	},
	{ "TIME",	m_time		},
	{ "ENCAP",	m_encap		},
	{ NULL,		NULL		}
};

#endif /* __HMSG_H_ */
