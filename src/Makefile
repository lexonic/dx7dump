COMPILER=g++
#OPTIONS=-g -std=c++17 -pedantic -Wall -Wextra -Werror -Wshadow -Wconversion -Wunreachable-code
#COMPILE=$(COMPILER) $(OPTIONS)
COMPILE=$(COMPILER)
INSTALL=install
PREFIX=~/.local/bin

# Compile main by default
all: clean dx7dump install

dx7dump: dx7dump.cpp
	$(COMPILE) -o $@ $<

install: installdirs
	$(INSTALL) -m 755 dx7dump $(DESTDIR)$(PREFIX)
	$(INSTALL) -m 755 dx7dumpall.sh $(DESTDIR)$(PREFIX)/dx7dumpall

installdirs:
	$(INSTALL) -d $(DESTDIR)$(PREFIX)

# Delete the program
clean:
	rm -f dx7dump

# These rules do not correspond to a specific file
.PHONY: install clean

