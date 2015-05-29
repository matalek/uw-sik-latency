TARGET: opoznienia

CC = g++
CXX = g++
CPPFLAGS = -std=c++11 -Wall -lboost_system -lboost_program_options

opoznienia: opoznienia.o
	$(CXX) $^ -o $@ $(CPPFLAGS)

.PHONY: clean TARGET
clean:
	rm -f opoznienia *.o *~ *.bak
