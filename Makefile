# change the following line to install to a different prefix
PREFIX = /usr/local

src = $(wildcard src/*.c)
obj = $(src:.c=.o)
dep = $(obj:.o=.d)
bin = dtms

dbg = -g
opt = -O0

CFLAGS = -pedantic -Wall -std=gnu99 -DPREFIX=\"$(PREFIX)\" $(dbg)

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(obj) $(bin) $(dep)

.PHONY: install
install: $(bin)
	mkdir -p $(PREFIX)/bin $(PREFIX)/share/dtms
	cp $(bin) $(PREFIX)/bin/$(bin)
	chown root $(PREFIX)/bin/$(bin)
	chmod +s $(PREFIX)/bin/$(bin)
	cp data/dtms.mp3 $(PREFIX)/share/dtms/dtms.mp3

.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/bin/$(bin)
	rm -f $(PREFIX)/share/dtms/dtms.mp3
	rmdir $(PREFIX)/share/dtms


