CFLAGS = -Wall -Werror -Wstrict-overflow -Wextra -fno-strict-aliasing -g -march=native
PREFIX=/usr/local





src = $(wildcard *.c)
obj = $(src:.c=.o)

LDFLAGS += `pkg-config --libs papi hwloc`
CFLAGS += `pkg-config --cflags papi hwloc`

aypapi: $(obj)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean install uninstall
clean:
	rm -f $(obj) aypapi

install: aypapi
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $< $(DESTDIR)$(PREFIX)/bin/aypapi

uninstall:
	rm -f $(PREFIX)/bin/aypapi
