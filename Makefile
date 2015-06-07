TARGET: opoznienia

CC = g++
CPP = g++
CPPFLAGS = -std=c++11 -Wall -lboost_system -lpthread

OBJECTS = opoznienia.o mdns_server.o shared.o mdns_fields.o name_server.o icmp_client.o computer.o

$(OBJECTS): %.o: %.cc
	$(CC) $< $(CPPFLAGS) -c

#opoznienia: opoznienia.o mdns_server.o shared.o mdns_fields.o name_server.o icmp_client.o computer.o
opoznienia: $(OBJECTS)
	$(CC) $^ -o $@ $(CPPFLAGS)

.PHONY: clean TARGET
clean:
	rm -f opoznienia *.o *~ *.bak



