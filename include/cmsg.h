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

#ifndef __CMSG_H_
#define __CMSG_H_
#include "services.h"

static struct message {
	char *cmd;
	int access;
	int sclient;
	int (*func)(int, client *, char *);
	void (*help)(client *, int);
} msgtab[] = {
	{ "HELP",	CLIENT_USER,	SCLIENT_ANY,	sc_help,	NULL		},
#if 0
// NickServ
	{ "REGISTER",	CLIENT_USER,	SCLIENT_NICK,	ns_register,	ns_register_help},
	{ "IDENTIFY",	CLIENT_USER,	SCLIENT_NICK,	ns_identify,	ns_identify_help},
	{ "DROP",	CLIENT_USER,	SCLIENT_NICK,	ns_drop,	ns_drop_help	},
	{ "INFO",	CLIENT_USER,	SCLIENT_NICK,	ns_info,	NULL		},
	{ "LIST",	CLIENT_OPER,	SCLIENT_NICK,	ns_list,	NULL		},

// ChanServ
	{ "LIST",	CLIENT_OPER,	SCLIENT_CHAN,	cs_list,	cs_list_help	},
	{ "WHO",	CLIENT_OPER,	SCLIENT_CHAN,	cs_who,		cs_who_help	},
#endif
// OperServ
	{ "RAW",	CLIENT_OPER,	SCLIENT_OPER,	os_raw,		NULL		},
	{ "OPME",	CLIENT_OPER,	SCLIENT_OPER,	os_opme,	NULL		},
	{ "STATUS",	CLIENT_OPER,	SCLIENT_OPER,	os_status,	NULL		},
	{ "GLINE",	CLIENT_OPER,	SCLIENT_OPER,	os_gline,	NULL		},
	{ "UNGLINE",	CLIENT_OPER,	SCLIENT_OPER,	os_ungline,	NULL		},
	{ "LONERS",	CLIENT_OPER,	SCLIENT_OPER,	os_loners,	NULL		},
	{ "STATUS",	CLIENT_OPER,	SCLIENT_OPER,	os_status,	NULL		},
	{ "WHO",	CLIENT_OPER,	SCLIENT_OPER,	os_who,		NULL		},
#if 0
	{ "JUPE",	CLIENT_OPER,	SCLIENT_OPER,	os_jupe,	NULL		},
	{ "CHAT",	CLIENT_OPER,	SCLIENT_OPER,	os_chat,	NULL		},
	{ "CLONES",	CLIENT_OPER,	SCLIENT_OPER,	os_clones,	NULL		},
	{ "KILLCLONES",	CLIENT_OPER,	SCLIENT_OPER,	os_killclones,	NULL		},
	{ "ACLONES",	CLIENT_OPER,	SCLIENT_OPER,	os_aclones,	NULL		},
	{ "MODE",	CLIENT_OPER,	SCLIENT_OPER,	os_mode,	os_mode_help	},
	{ "KICK",	CLIENT_OPER,	SCLIENT_OPER,	os_kick,	os_kick_help	},
	{ "KILL",	CLIENT_OPER,	SCLIENT_OPER,	os_kill,	os_kill_help	},
	{ "SETTIME",	CLIENT_OPER,	SCLIENT_OPER,	os_settime,	NULL		},
	{ "PLACEHOLD",	CLIENT_OPER,	SCLIENT_OPER,	os_placehold,	NULL		},
	{ "KLINE",	CLIENT_OPER,	SCLIENT_OPER,	os_kline,	NULL		},
	{ "ANONGLINE",	CLIENT_OPER,	SCLIENT_OPER,	os_anongline,	os_gline_help	},
	{ "SNUGGLE",	CLIENT_OPER,	SCLIENT_OPER,	os_snuggle,	os_snuggle_help	},
	{ "ACLONE",	CLIENT_OPER,	SCLIENT_OPER,	os_aclone,	os_aclone_help	},
#endif
	{ (char *)NULL,	(int)NULL,	0,	NULL,		NULL		}
};

#endif /* __CMSG_H_ */
