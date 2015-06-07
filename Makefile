TARGET: opoznienia

CPP = g++
CPPFLAGS = -std=c++0x -Wall -lboost_system -lpthread

OBJECTS = opoznienia.o err.o drop_to_nobody.o mdns_server.o shared.o mdns_fields.o name_server.o icmp_client.o computer.o

$(OBJECTS): %.o: %.cc
	$(CPP) $< $(CPPFLAGS) -c

#opoznienia: opoznienia.o mdns_server.o shared.o mdns_fields.o name_server.o icmp_client.o computer.o
opoznienia: $(OBJECTS)
	$(CPP) $^ -o $@ $(CPPFLAGS)

.PHONY: clean TARGET
clean:
	rm -f opoznienia *.o *~ *.bak



