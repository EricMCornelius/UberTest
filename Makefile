example.tsk | example.cpp:
	g++ -std=c++11 example.cpp -I. -o example.tsk

clean:
	rm -f example.tsk
