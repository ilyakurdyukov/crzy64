ARCH := $(shell uname -m)

APPNAME := crzy64
SRCNAME := main.c

ifneq (,$(filter arm% aarch64,$(ARCH)))
	MFLAGS := -mcpu=native
else
	MFLAGS := -march=native
endif

CFLAGS := -Wall -Wextra -pedantic -O2 $(MFLAGS)
SFLAGS := -fno-asynchronous-unwind-tables

ifneq (,$(filter e2k,$(ARCH)))
	CFLAGS := $(filter-out -O2,$(CFLAGS)) -O3
endif
ifneq (,$(filter i386 i686 x86_64,$(ARCH)))
	SFLAGS += -masm=intel
endif

.PHONY: clean all check bench

all: $(APPNAME)

clean:
	rm -f $(APPNAME) crzy64_test crzy64_bench

$(APPNAME): $(SRCNAME) crzy64.h
	$(CC) $(CFLAGS) -s -o $@ $<

$(APPNAME).s: $(SRCNAME) crzy64.h
	$(CC) $(CFLAGS) $(SFLAGS) -S -o $@ $<

crzy64_%: %.c crzy64.h
	$(CC) $(CFLAGS) -s -o $@ $< -lm

check: crzy64_test
	./crzy64_test

bench: crzy64_bench
	./crzy64_bench $(BARG)

