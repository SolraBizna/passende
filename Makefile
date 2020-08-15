all: passendetest

passendetest: test.cc passende.cc passende.hh
	c++ -std=c++11 -Og -g -Wall test.cc passende.cc -o passendetest

clean:
	rm -f passendetest *~ \#*\#

