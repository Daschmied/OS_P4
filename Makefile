all: site-tester

site-tester: site-tester.cpp
	g++ -Wall -g -lcurl site-tester.cpp -o site-tester

clean:
	rm site-tester
