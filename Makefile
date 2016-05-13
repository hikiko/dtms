src = $(wildcard src/*.c)
obj = $(src:.c=.o)
dep = $(obj:.o=.d)
bin = dtms

dbg = -g
opt = -O0

CC = gcc
INCLUDE = -I/usr/include
CFLAGS = -pedantic -Wall -std=c99 -D_XOPEN_SOURCE=600 $(dbg) $(INCLUDE)
LDFLAGS = -L/usr/lib $(libs)

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(obj) $(bin) $(dep)
