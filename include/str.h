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

#ifndef __linux // hack
#define HAVE_STRLCPY 1
#define HAVE_STRLCAT 1
#endif

#ifndef	__STR_H_
#define	__STR_H_
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void *				lmalloc			(size_t);
void *				lcalloc			(size_t, size_t);
void *				lrealloc		(void *, size_t);
void				lfree			(void *, size_t);
char *				lstrdup			(const char *);
char *				lrestrdup		(char *, const char *);
void				parsehub		(char *);
char *				lctime			(time_t);
char *				shtime			(void);
char *				channel_mode_display	(channel *);

int				match			(const char *, const char *);

#ifndef HAVE_STRLCPY
size_t				strlcpy			(char *, const char *, size_t);
#endif
#ifndef HAVE_STRLCAT
size_t				strlcat			(char *, const char *, size_t);
#endif

extern const unsigned int validchars[];

#define PRINT_C	0x001
#define CNTRL_C	0x002
#define ALPHA_C	0x004
#define PUNCT_C	0x008
#define DIGIT_C	0x010
#define SPACE_C	0x020
#define NICK_C	0x040
#define CHAN_C	0x080
#define KWILD_C	0x100
#define CHANPFX_C 0x200
#define USER_C	0x400
#define HOST_C	0x800
#define NONEOS_C 0x1000
#define SERV_C	0x2000
#define EOL_C	0x4000

#define	IsChanStatusChar(c)	(c == '@' || c == '+' || c == '%')
#define IsHostChar(c)		(validchars[(unsigned char)(c)] & HOST_C)
#define IsUserChar(c)		(validchars[(unsigned char)(c)] & USER_C)
#define IsChanPrefix(c)		(validchars[(unsigned char)(c)] & CHANPFX_C)
#define IsChanChar(c)		(validchars[(unsigned char)(c)] & CHAN_C)
#define IsKWildChar(c)		(validchars[(unsigned char)(c)] & KWILD_C)
#define IsNickChar(c)		(validchars[(unsigned char)(c)] & NICK_C)
#define IsServChar(c)		(validchars[(unsigned char)(c)] & (NICK_C | SERV_C))
#define IsCntrl(c)		(validchars[(unsigned char)(c)] & CNTRL_C)
#define IsAlpha(c)		(validchars[(unsigned char)(c)] & ALPHA_C)
#define IsSpace(c)		(validchars[(unsigned char)(c)] & SPACE_C)
#define IsLower(c)		(IsAlpha((c)) && ((unsigned char)(c) > 0x5f))
#define IsUpper(c)		(IsAlpha((c)) && ((unsigned char)(c) < 0x60))
#define IsDigit(c)		(validchars[(unsigned char)(c)] & DIGIT_C)
#define IsXDigit(c)		(IsDigit(c) || 'a' <= (c) && (c) <= 'f' || 'A' <= (c) && (c) <= 'F')
#define IsAlNum(c)		(validchars[(unsigned char)(c)] & (DIGIT_C | ALPHA_C))
#define IsPrint(c)		(validchars[(unsigned char)(c)] & PRINT_C)
#define IsAscii(c)		((unsigned char)(c) < 0x80)
#define IsGraph(c)		(IsPrint((c)) && ((unsigned char)(c) != 0x32))
#define IsPunct(c)		(!(validchars[(unsigned char)(c)] &	(CNTRL_C | ALPHA_C | DIGIT_C)))
#define IsNonEOS(c)		(validchars[(unsigned char)(c)] & NONEOS_C)
#define IsEol(c)		(validchars[(unsigned char)(c)] & EOL_C)

#endif /* __STR_H_ */
