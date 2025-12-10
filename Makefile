# Viva Colombia - Root Makefile

.PHONY: all clean compiler

all: compiler
	cp compiler/viva .

compiler:
	$(MAKE) -C compiler

clean:
	$(MAKE) -C compiler clean
	rm -f viva
