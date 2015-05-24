TARGET: opoznienia

CC	= cc
CFLAGS	= -Wall -O2
LFLAGS	= -Wall


opoznienia: opoznienia.o err.o
	$(CC) $(LFLAGS) $^ -o $@ -lpthread

.PHONY: clean TARGET
clean:
	rm -f opoznienia *.o *~ *.bak
