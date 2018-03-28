all:
	g++ raspicam_wrapper.cpp -c -o raspicam_wrapper.o
	gcc simpletest_raspicam_wrapper.c -c -o simpletest_raspicam_wrapper.o
	g++ raspicam_wrapper.o simpletest_raspicam_wrapper.o -o simpletest_raspicam -lraspicam

clean:
	rm simpletest_raspicam *.o
