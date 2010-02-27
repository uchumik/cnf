# General makefile

MAKE=make
SHELL=/bin/sh

SUBDIRS=src

world: check all
	@echo "================================================"
	@echo "CONGRATULATIONS: The compilation was successful."
	@echo "To know what to do next, check the README file."
	@echo "================================================"

all clean:
	@for n in ${SUBDIRS};\
		do ( cd $$n && ${MAKE} ${@}) || exit ; done

all: check

.PHONY: world all depend check
