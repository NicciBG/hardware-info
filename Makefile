CC       ?= gcc
STD      := -std=c23
WARN     := -Wall -Wextra -Wpedantic -Wconversion
CFLAGS   ?= $(STD) $(WARN) -O2
TESTFLAGS ?= $(STD) $(WARN) -g -fsanitize=address,undefined

LIBS :=
TESTS :=
PROBES :=

include build/*.mk

.PHONY: all libs probes test clean

all: libs probes

libs: $(LIBS)

probes: $(PROBES)

test: $(TESTS)

clean:
	rm -f $(LIBS) $(PROBES)
	rm -rf build/bin
