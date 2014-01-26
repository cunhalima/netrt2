#===========================================================================
#
# netrt
# Copyright (C) 2013 Alex Reimann Cunha Lima, Andrey Fagundes
#
# This file is part of the netrt
#
# netrt is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# netrt is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with MDB Modular Database System. If not, see
# <http://www.gnu.org/licenses/>.
#
#===========================================================================
PRJ=netrt
BDIR=bin
ODIR=obj
SDIR=src
CC=gcc
CFLAGS=-O2 -g -Wall -Wextra -Werror
CFLAGS+=-Wno-unused-parameter -Wno-unused-variable
CFLAGS+=-Wno-unused-but-set-variable
CFLAGS+=-std=c99 -pedantic
LDFLAGS=-lrt
_MAINTEST=main.o
_MODULES=szb.o console.o timer.o net.o options.o tables.o alltabs.o router.o reliable.o command.o
MAINTEST=$(patsubst %, $(ODIR)/%, $(_MAINTEST))
MODULES=$(patsubst %, $(ODIR)/%, $(_MODULES))
OBJS=$(MAINTEST) $(MODULES)
EXE=$(BDIR)/$(PRJ)
TARBALL=$(PRJ).tar.gz

.PHONY: all clean run val dist test

all: $(EXE)

$(EXE): $(MAINTEST) $(MODULES)
	@$(CC) $^ $(LDFLAGS) -o $@

$(ODIR)/%.o: $(SDIR)/%.c
	@$(CC) -c $< $(CFLAGS) -o $@

$(EXE): | $(BDIR)

$(BDIR):
	@mkdir -p $(BDIR)

$(OBJS): | $(ODIR)

$(ODIR):
	@mkdir -p $(ODIR)

clean:
	@rm -f $(EXE) $(OBJS)

run: $(EXE)
	@./$(EXE) -v -c -s

val: $(EXE)
	@valgrind --tool=memcheck --error-limit=no --leak-check=full --show-reachable=yes ./$(EXE) 1
#	//@valgrind --tool=memcheck ./$(EXE)
#valgrind --tool=memcheck --track-origins=yes -v ./$^ 1

dist: $(TARBALL)

$(TARBALL):
	tar -cvzf $@ README.md LICENSE src doc \
	Makefile autoexec.script enlaces.config roteador.config

test: $(EXE)
	@xterm -geometry +000+0 -e "./bin/netrt 1 -e 20" &
	@xterm -geometry +490+0 -e "./bin/netrt 2 -e 33" &
	@xterm -geometry +980+0 -e "./bin/netrt 3 -e 20" &
	@xterm -geometry +0+360 -e "./bin/netrt 4 -e 30" &
	@xterm -geometry +490+360 -e "./bin/netrt 5 -e 20" &
	@xterm -geometry +980+360 -e "./bin/netrt 6 -e 20" &
