#
# zorak services - a framework for implementing irc network service bots
# Copyright 2002-2015 J. Maurice <j@wiz.biz>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#
#  $Id: Makefile.in,v 1.22 2002/07/30 21:31:15 wiz Exp $
#
CC=	gcc
RM=	/bin/rm
MKDEP=	gcc -MM
#SSL_LIBS=  -lcrypto
#SSL_INCLUDES=

# leet cflags
CPPFLAGS=-I../include $(SSL_INCLUDES)
CFLAGS	=-pipe -O2 -ggdb -Wunused -Wall -Wmissing-declarations
LIBS	=
OUT		=zorakd

SRCS  =	\
	irc.c\
	main.c\
	net.c\
	state.c\
	str.c\
	services.c\


OBJS=${SRCS:.c=.o}

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

all: zorak

zorak: $(OBJS)
	$(CC) $(CFLAGS) -o $(OUT) $(OBJS) $(LIBS) $(SSL_LIBS)

clean:
	$(RM) -f $(OBJS) $(OUT) *.core

depend:
	$(MKDEP) $(CPPFLAGS) $(SRCS) > .depend

distclean: clean

install:
	install -c -m700 $(OUT) /home/jmaurice/zorak/

include .depend
