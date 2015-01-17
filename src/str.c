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
#include <syslog.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "main.h"
#include "str.h"
#include "irc.h"
#include "hmsg.h"

extern main_t me;

void *
lmalloc(size_t size)
{
	void *ptr = malloc(size);

	if (!ptr) {
		logger(LOG_CRIT, "lmalloc(%d): fail!", size);
		return NULL;
	}
	me.memusage += size;

	return ptr;
}

void *
lcalloc(size_t number, size_t size)
{
	void *ptr = calloc(number, size);

	if (!ptr) {
		logger(LOG_CRIT, "lcalloc(%d): fail!", size);
		return NULL;
	}
	me.memusage += (number * size);

	return ptr;
}

void *
lrealloc(void *ptr, size_t size)
{
	void *tmp;

	me.memusage -= strlen(ptr) + 1;
	if (!(tmp = realloc(ptr, size))) {
		logger(LOG_CRIT, "lrealloc(%d): fail!", size);
		return NULL;
	}
	me.memusage += size;

	return tmp;
}

void
lfree(void *ptr, size_t size)
{
	me.memusage -= size;
	free(ptr);
}

char *
lstrdup(const char *in)
{
	char *out = lmalloc(strlen(in) + 1);
	strlcpy(out, in, strlen(in) + 1);

	return out;
}

char *
lrestrdup(char *old, const char *new)
{
	char *result = lrealloc(old, strlen(new) + 1);
	strlcpy(result, new, strlen(new) + 1);

	return result;
}

char *
lctime(time_t lclock)
{
	static char buf[25];

	strlcpy(buf, ctime(&lclock), 25);
	return buf;
}

char *
shtime(void)
{
	static char buf[9];
	strlcpy(buf, lctime(me.now.tv_sec) + 11, 9);
#if 0
	strlcpy(buf, lctime(me.now.tv_sec) + 11, 6);
#endif
	return buf;
}

char *
channel_mode_display(channel *chan)
{
	static char modestr[BUFSIZE];
	char tmp[BUFSIZE];

	strlcpy(modestr, "+", BUFSIZE);

	// imnpst
	if (IsSetBitMask(chan->flags, CMODE_i))
		strlcat(modestr, "i", BUFSIZE);
	if (IsSetBitMask(chan->flags, CMODE_m))
		strlcat(modestr, "m", BUFSIZE);
	if (IsSetBitMask(chan->flags, CMODE_n))
		strlcat(modestr, "n", BUFSIZE);
	if (IsSetBitMask(chan->flags, CMODE_p))
		strlcat(modestr, "p", BUFSIZE);
	if (IsSetBitMask(chan->flags, CMODE_s))
		strlcat(modestr, "s", BUFSIZE);
	if (IsSetBitMask(chan->flags, CMODE_S))
		strlcat(modestr, "S", BUFSIZE);
	if (IsSetBitMask(chan->flags, CMODE_t))
		strlcat(modestr, "t", BUFSIZE);
	if (IsSetBitMask(chan->flags, CMODE_k))
		strlcat(modestr, "k", BUFSIZE);
	if (IsSetBitMask(chan->flags, CMODE_l))
		strlcat(modestr, "l", BUFSIZE);

	// key and limit
	if (IsSetBitMask(chan->flags, CMODE_k)) {
		strlcat(modestr, " ", BUFSIZE);
		strlcat(modestr, chan->key, BUFSIZE);
	}
	if (IsSetBitMask(chan->flags, CMODE_l)) {
		strlcat(modestr, " ", BUFSIZE);
		snprintf(tmp, BUFSIZE, "%d", chan->memberlimit);
		strlcat(modestr, tmp, BUFSIZE);
	}

	return modestr;
}

void
parsehub(char *buf)
{
#define PARVMAX 15
	struct command *mptr = cmdtab;
	static char *parv[PARVMAX + 2];
	char *tmp, *s;
	client *cptr = NULL;
	int i;

	debug(3, "parsehub: %s", buf);

	for (i = 0; i < PARVMAX; i++)
		parv[i] = NULL;

	if (*buf == ':')
	{
		s = &buf[1];
		if(!(*s))
			return;

		parv[0] = s;

		if(!(s = strchr((char *)&buf[1], ' ')))
			return;

		*s++ = '\0';

	}
	else
		s = &buf[0];

	if((tmp = (char *)index(s, ' ')))
		*tmp++ = '\0';

	for (; mptr->cmd; mptr++)
		if (strcmp(mptr->cmd, s) == 0)
			goto next;

	debug(3, "parsehub: unknown command %s, ignoring line", s);
	return;
next:
	if (!(s = tmp))
		goto doit;
	i = 0;

	for (;;)
	{
		while (*s == ' ')
			*s++ = '\0';

		if (*s == '\0')
			break;

		if (*s == ':')
		{
			parv[++i] = s + 1;
			break;
		}

		parv[++i] = s;

		if (i >= PARVMAX)
			break;

		for (; *s != ' ' && *s; s++);
	}

	if (parv[0] && !(cptr = client_list_search(parv[0])))
		return;
doit:
	debug(6, "parsehub: executing %s(argc = %d)", mptr->cmd, i + 1);
	(*mptr->func)(cptr, i + 1, parv);
}


/* strlcpy() / strlcat() taken from FreeBSD libc */

#ifndef HAVE_STRLCPY
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;

	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	if (n == 0) {
		if (siz != 0)
			*d = '\0';
		while (*s++)
			;
	}

	return(s - src - 1);
}
#endif
#ifndef HAVE_STRLCAT
size_t
strlcat(char *dst, const char *src, size_t siz)
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;
	size_t dlen;

	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));
}
#endif

/* match() originally written by Douglas A Lewis (dalewis@acsu.buffalo.edu) */
int
match(const char *mask, const char *name)
{
	const unsigned char *m = (const unsigned char *)mask;
	const unsigned char *n = (const unsigned char *)name;
	const unsigned char *ma = (const unsigned char *)mask;
	const unsigned char *na = (const unsigned char *)name;
	int wild = 0;

loop:
	if (*m == '*') {
		while (*m == '*')
			m++;
		wild = 1;
		ma = m;
		na = n;
	}
	if (!*m) {
		if (!*n)
			return 1;
		for (m--; (m > (const unsigned char *)mask) && (*m == '?'); m--) ;
		if (*m == '*' && (m > (const unsigned char *) mask))
			return 1;
		if (!wild)
			return 0;
		m = ma;
		n = ++na;
	} else if (!*n) {
		while (*m == '*')
			m++;
		return (*m == 0);
	}
	if (tolower(*m) != tolower(*n) && *m != '?') {
		if (!wild)
			return 0;
		m = ma;
		n = ++na;
	} else {
		if (*m)
			m++;
		if (*n)
			n++;
	}
	goto loop;
}

/* validchars array taken from hybrid 5 */

const unsigned int validchars[] = {
/* 0  */     CNTRL_C,
/* 1  */     CNTRL_C|CHAN_C|NONEOS_C,
/* 2  */     CNTRL_C|CHAN_C|NONEOS_C,
/* 3  */     CNTRL_C|CHAN_C|NONEOS_C,
/* 4  */     CNTRL_C|CHAN_C|NONEOS_C,
/* 5  */     CNTRL_C|CHAN_C|NONEOS_C,
/* 6  */     CNTRL_C|CHAN_C|NONEOS_C,
/* 7 BEL */  CNTRL_C|NONEOS_C,
/* 8  \b */  CNTRL_C|CHAN_C|NONEOS_C,
/* 9  \t */  CNTRL_C|SPACE_C|CHAN_C|NONEOS_C,
/* 10 \n */  CNTRL_C|SPACE_C|CHAN_C|NONEOS_C|EOL_C,
/* 11 \v */  CNTRL_C|SPACE_C|CHAN_C|NONEOS_C,
/* 12 \f */  CNTRL_C|SPACE_C|CHAN_C|NONEOS_C,
/* 13 \r */  CNTRL_C|SPACE_C|CHAN_C|NONEOS_C|EOL_C,
/* 14 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 15 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 16 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 17 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 18 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 19 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 20 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 21 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 22 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 23 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 24 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 25 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 26 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 27 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 28 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 29 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 30 */     CNTRL_C|CHAN_C|NONEOS_C,
/* 31 */     CNTRL_C|CHAN_C|NONEOS_C,
/* SP */     PRINT_C|SPACE_C,
/* ! */      PRINT_C|KWILD_C|CHAN_C|NONEOS_C,
/* " */      PRINT_C|CHAN_C|NONEOS_C,
/* # */      PRINT_C|CHANPFX_C|CHAN_C|NONEOS_C,
/* $ */      PRINT_C|CHAN_C|NONEOS_C|USER_C,
/* % */      PRINT_C|CHAN_C|NONEOS_C,
/* & */      PRINT_C|CHANPFX_C|CHAN_C|NONEOS_C,
/* ' */      PRINT_C|CHAN_C|NONEOS_C,
/* ( */      PRINT_C|CHAN_C|NONEOS_C,
/* ) */      PRINT_C|CHAN_C|NONEOS_C,
/* * */      PRINT_C|KWILD_C|CHAN_C|NONEOS_C|SERV_C,
/* + */      PRINT_C|CHAN_C|NONEOS_C,
/* , */      PRINT_C|NONEOS_C,
/* - */      PRINT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* . */      PRINT_C|KWILD_C|CHAN_C|NONEOS_C|USER_C|HOST_C|SERV_C,
/* / */      PRINT_C|CHAN_C|NONEOS_C,
/* 0 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 1 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 2 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 3 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 4 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 5 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 6 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 7 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 8 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* 9 */      PRINT_C|DIGIT_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* : */      PRINT_C|CHAN_C|NONEOS_C,
/* ; */      PRINT_C|CHAN_C|NONEOS_C,
/* < */      PRINT_C|CHAN_C|NONEOS_C,
/* = */      PRINT_C|CHAN_C|NONEOS_C,
/* > */      PRINT_C|CHAN_C|NONEOS_C,
/* ? */      PRINT_C|KWILD_C|CHAN_C|NONEOS_C,
/* @ */      PRINT_C|KWILD_C|CHAN_C|NONEOS_C,
/* A */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* B */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* C */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* D */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* E */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* F */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* G */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* H */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* I */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* J */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* K */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* L */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* M */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* N */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* O */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* P */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* Q */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* R */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* S */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* T */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* U */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* V */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* W */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* X */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* Y */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* Z */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* [ */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
/* \ */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
/* ] */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
/* ^ */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
/* _ */      PRINT_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
/* ` */      PRINT_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
/* a */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* b */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* c */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* d */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* e */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* f */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* g */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* h */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* i */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* j */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* k */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* l */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* m */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* n */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* o */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* p */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* q */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* r */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* s */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* t */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* u */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* v */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* w */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* x */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* y */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* z */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C|HOST_C,
/* { */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
/* | */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
/* } */      PRINT_C|ALPHA_C|NICK_C|CHAN_C|NONEOS_C|USER_C,
/* ~ */      PRINT_C|ALPHA_C|CHAN_C|NONEOS_C|USER_C,
/* del  */   CHAN_C|NONEOS_C,
/* 0x80 */   CHAN_C|NONEOS_C,
/* 0x81 */   CHAN_C|NONEOS_C,
/* 0x82 */   CHAN_C|NONEOS_C,
/* 0x83 */   CHAN_C|NONEOS_C,
/* 0x84 */   CHAN_C|NONEOS_C,
/* 0x85 */   CHAN_C|NONEOS_C,
/* 0x86 */   CHAN_C|NONEOS_C,
/* 0x87 */   CHAN_C|NONEOS_C,
/* 0x88 */   CHAN_C|NONEOS_C,
/* 0x89 */   CHAN_C|NONEOS_C,
/* 0x8A */   CHAN_C|NONEOS_C,
/* 0x8B */   CHAN_C|NONEOS_C,
/* 0x8C */   CHAN_C|NONEOS_C,
/* 0x8D */   CHAN_C|NONEOS_C,
/* 0x8E */   CHAN_C|NONEOS_C,
/* 0x8F */   CHAN_C|NONEOS_C,
/* 0x90 */   CHAN_C|NONEOS_C,
/* 0x91 */   CHAN_C|NONEOS_C,
/* 0x92 */   CHAN_C|NONEOS_C,
/* 0x93 */   CHAN_C|NONEOS_C,
/* 0x94 */   CHAN_C|NONEOS_C,
/* 0x95 */   CHAN_C|NONEOS_C,
/* 0x96 */   CHAN_C|NONEOS_C,
/* 0x97 */   CHAN_C|NONEOS_C,
/* 0x98 */   CHAN_C|NONEOS_C,
/* 0x99 */   CHAN_C|NONEOS_C,
/* 0x9A */   CHAN_C|NONEOS_C,
/* 0x9B */   CHAN_C|NONEOS_C,
/* 0x9C */   CHAN_C|NONEOS_C,
/* 0x9D */   CHAN_C|NONEOS_C,
/* 0x9E */   CHAN_C|NONEOS_C,
/* 0x9F */   CHAN_C|NONEOS_C,
/* 0xA0 */   CHAN_C|NONEOS_C,
/* 0xA1 */   CHAN_C|NONEOS_C,
/* 0xA2 */   CHAN_C|NONEOS_C,
/* 0xA3 */   CHAN_C|NONEOS_C,
/* 0xA4 */   CHAN_C|NONEOS_C,
/* 0xA5 */   CHAN_C|NONEOS_C,
/* 0xA6 */   CHAN_C|NONEOS_C,
/* 0xA7 */   CHAN_C|NONEOS_C,
/* 0xA8 */   CHAN_C|NONEOS_C,
/* 0xA9 */   CHAN_C|NONEOS_C,
/* 0xAA */   CHAN_C|NONEOS_C,
/* 0xAB */   CHAN_C|NONEOS_C,
/* 0xAC */   CHAN_C|NONEOS_C,
/* 0xAD */   CHAN_C|NONEOS_C,
/* 0xAE */   CHAN_C|NONEOS_C,
/* 0xAF */   CHAN_C|NONEOS_C,
/* 0xB0 */   CHAN_C|NONEOS_C,
/* 0xB1 */   CHAN_C|NONEOS_C,
/* 0xB2 */   CHAN_C|NONEOS_C,
/* 0xB3 */   CHAN_C|NONEOS_C,
/* 0xB4 */   CHAN_C|NONEOS_C,
/* 0xB5 */   CHAN_C|NONEOS_C,
/* 0xB6 */   CHAN_C|NONEOS_C,
/* 0xB7 */   CHAN_C|NONEOS_C,
/* 0xB8 */   CHAN_C|NONEOS_C,
/* 0xB9 */   CHAN_C|NONEOS_C,
/* 0xBA */   CHAN_C|NONEOS_C,
/* 0xBB */   CHAN_C|NONEOS_C,
/* 0xBC */   CHAN_C|NONEOS_C,
/* 0xBD */   CHAN_C|NONEOS_C,
/* 0xBE */   CHAN_C|NONEOS_C,
/* 0xBF */   CHAN_C|NONEOS_C,
/* 0xC0 */   CHAN_C|NONEOS_C,
/* 0xC1 */   CHAN_C|NONEOS_C,
/* 0xC2 */   CHAN_C|NONEOS_C,
/* 0xC3 */   CHAN_C|NONEOS_C,
/* 0xC4 */   CHAN_C|NONEOS_C,
/* 0xC5 */   CHAN_C|NONEOS_C,
/* 0xC6 */   CHAN_C|NONEOS_C,
/* 0xC7 */   CHAN_C|NONEOS_C,
/* 0xC8 */   CHAN_C|NONEOS_C,
/* 0xC9 */   CHAN_C|NONEOS_C,
/* 0xCA */   CHAN_C|NONEOS_C,
/* 0xCB */   CHAN_C|NONEOS_C,
/* 0xCC */   CHAN_C|NONEOS_C,
/* 0xCD */   CHAN_C|NONEOS_C,
/* 0xCE */   CHAN_C|NONEOS_C,
/* 0xCF */   CHAN_C|NONEOS_C,
/* 0xD0 */   CHAN_C|NONEOS_C,
/* 0xD1 */   CHAN_C|NONEOS_C,
/* 0xD2 */   CHAN_C|NONEOS_C,
/* 0xD3 */   CHAN_C|NONEOS_C,
/* 0xD4 */   CHAN_C|NONEOS_C,
/* 0xD5 */   CHAN_C|NONEOS_C,
/* 0xD6 */   CHAN_C|NONEOS_C,
/* 0xD7 */   CHAN_C|NONEOS_C,
/* 0xD8 */   CHAN_C|NONEOS_C,
/* 0xD9 */   CHAN_C|NONEOS_C,
/* 0xDA */   CHAN_C|NONEOS_C,
/* 0xDB */   CHAN_C|NONEOS_C,
/* 0xDC */   CHAN_C|NONEOS_C,
/* 0xDD */   CHAN_C|NONEOS_C,
/* 0xDE */   CHAN_C|NONEOS_C,
/* 0xDF */   CHAN_C|NONEOS_C,
/* 0xE0 */   CHAN_C|NONEOS_C,
/* 0xE1 */   CHAN_C|NONEOS_C,
/* 0xE2 */   CHAN_C|NONEOS_C,
/* 0xE3 */   CHAN_C|NONEOS_C,
/* 0xE4 */   CHAN_C|NONEOS_C,
/* 0xE5 */   CHAN_C|NONEOS_C,
/* 0xE6 */   CHAN_C|NONEOS_C,
/* 0xE7 */   CHAN_C|NONEOS_C,
/* 0xE8 */   CHAN_C|NONEOS_C,
/* 0xE9 */   CHAN_C|NONEOS_C,
/* 0xEA */   CHAN_C|NONEOS_C,
/* 0xEB */   CHAN_C|NONEOS_C,
/* 0xEC */   CHAN_C|NONEOS_C,
/* 0xED */   CHAN_C|NONEOS_C,
/* 0xEE */   CHAN_C|NONEOS_C,
/* 0xEF */   CHAN_C|NONEOS_C,
/* 0xF0 */   CHAN_C|NONEOS_C,
/* 0xF1 */   CHAN_C|NONEOS_C,
/* 0xF2 */   CHAN_C|NONEOS_C,
/* 0xF3 */   CHAN_C|NONEOS_C,
/* 0xF4 */   CHAN_C|NONEOS_C,
/* 0xF5 */   CHAN_C|NONEOS_C,
/* 0xF6 */   CHAN_C|NONEOS_C,
/* 0xF7 */   CHAN_C|NONEOS_C,
/* 0xF8 */   CHAN_C|NONEOS_C,
/* 0xF9 */   CHAN_C|NONEOS_C,
/* 0xFA */   CHAN_C|NONEOS_C,
/* 0xFB */   CHAN_C|NONEOS_C,
/* 0xFC */   CHAN_C|NONEOS_C,
/* 0xFD */   CHAN_C|NONEOS_C,
/* 0xFE */   CHAN_C|NONEOS_C,
/* 0xFF */   CHAN_C|NONEOS_C
};

