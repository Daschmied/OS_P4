all: site-tester

site-tester: site-tester.cpp hqueue.h
	g++ -Wall -g -lcurl site-tester.cpp hqueue.h -o site-tester

clean:
	rm site-tester
