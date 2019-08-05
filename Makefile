CFLAGS = -Wall -Werror -Wstrict-overflow -Wextra -fno-strict-aliasing -g -march=native
PREFIX=/usr/local

obj = aypapi.o meters.o util.o args.o regulator.o
LDFLAGS += `pkg-config --libs papi hwloc`
CFLAGS += `pkg-config --cflags papi hwloc`

ifndef arch
$(error arch is not set. Please specify it using make arch=<your arch>)
endif

obj += arch/$(arch).o

aypapi: $(obj) 
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean install uninstall
clean:
	rm -f $(obj) aypapi

install:
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $< $(DESTDIR)$(PREFIX)/bin/aypapi

uninstall:
	rm -f $(PREFIX)/bin/aypapi
