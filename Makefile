APPNAME := crzy64
SRCNAME := main.c

MFLAGS := -march=native
CFLAGS := -Wall -Wextra -pedantic -O3 $(MFLAGS)
SFLAGS := -fno-asynchronous-unwind-tables -masm=intel

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
	./crzy64_bench

