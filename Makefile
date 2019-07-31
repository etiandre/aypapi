src = $(wildcard *.c)
obj = $(src:.c=.o)

LDFLAGS = -lpapi -lhwloc
CFLAGS = -Wall -Werror -Wstrict-overflow -Wextra -fno-strict-aliasing \
-g \
-march=native

aypapi: $(obj)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) aypapi
