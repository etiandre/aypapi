src = $(wildcard *.c)
obj = $(src:.c=.o)

LDFLAGS = -lpapi
CFLAGS = -Wall -Werror -g
aypapi: $(obj)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) aypapi
