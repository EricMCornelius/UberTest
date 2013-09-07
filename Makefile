example.tsk | example.cpp:
	g++-4.8 -std=c++11 -pthread example.cpp -I. -o example.tsk

clean:
	rm -f example.tsk
