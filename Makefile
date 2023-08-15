all:
	g++ -c bridge.cc -lcurl
	g++ bridge.o -o bridge -lcurl
	rm bridge.o
