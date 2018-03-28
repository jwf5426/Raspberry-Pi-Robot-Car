all:
	gcc import_registers.c -c
	gcc accelerometer_project.c -c
	gcc wait_key.c -c
	gcc hw7frazier.c -c 
	gcc hw7frazier_camera.c -c
	g++ camera_project/raspicam_wrapper.cpp -c
	gcc LowerResolution/scale_image_data.c -c
	g++ raspicam_wrapper.o hw7frazier.o hw7frazier_camera.o import_registers.o accelerometer_project.o wait_key.o scale_image_data.o -o hw7frazier -lraspicam -lwiringPi -lpthread -lm
clean: 
	rm import_registers.o accelerometer_project.o wait_key.o hw7frazier.o scale_image_data.o raspicam_wrapper.o hw7frazier

