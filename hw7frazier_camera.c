#include "hw7frazier_camera.h"

int initCamera() {
	Camera = raspicam_wrapper_create();
	if (Camera != NULL)
	{
		if (raspicam_wrapper_open( Camera ))
		{
			// wait a while until camera stabilizes
			// printf( "Sleeping for 3 secs\n" );
			sleep( 3 );
		}
		else
		{
			printf( "Error opening camera\n" );
			return -1;
		}
	}
	else
	{
		printf( "Unable to allocate camera handle\n" );
		return -1;
	}	
	return 0;
}


int destoryCamera() {
	raspicam_wrapper_destroy( Camera );
	return 0;
}

int captureCamera(int count) {
	int returnValue = -1;
	
	 // capture
	raspicam_wrapper_grab( Camera );

	// allocate memory
	unsigned char *data = (unsigned char *)malloc( raspicam_wrapper_getImageTypeSize( Camera, RASPICAM_WRAPPER_FORMAT_RGB ) );
	
	// extract the image in rgb format
	raspicam_wrapper_retrieve( Camera, data, RASPICAM_WRAPPER_FORMAT_RGB ); // get camera image
	
	/* save
	char buf[0x100];
	snprintf(buf, sizeof(buf), "raspicam_image%d.ppm", count);

	FILE * outFile = fopen( buf, "wb" );
	if (outFile != NULL)
	{*/
		struct pixel_format_RGB* pixel;
		unsigned int      pixel_count;
		unsigned int      pixel_index;
		unsigned char     pixel_value;

		pixel = (struct pixel_format_RGB *)data;
		pixel_count = raspicam_wrapper_getHeight( Camera ) * raspicam_wrapper_getWidth( Camera );
		for (pixel_index = 0; pixel_index < pixel_count; pixel_index++)
		{
			pixel_value = (((unsigned int)(pixel[pixel_index].R)) +
			((unsigned int)(pixel[pixel_index].G)) +
			((unsigned int)(pixel[pixel_index].B))) / 3; // do not worry about rounding
			pixel[pixel_index].R = pixel_value;
			pixel[pixel_index].G = pixel_value;
			pixel[pixel_index].B = pixel_value;
		}
		
		unsigned int scaleFactor = 2;
		struct pixel_format_RGB* scaledData = malloc(raspicam_wrapper_getImageTypeSize( Camera, RASPICAM_WRAPPER_FORMAT_RGB ) / scaleFactor);
		scale_image_data(pixel, raspicam_wrapper_getHeight( Camera ), raspicam_wrapper_getWidth( Camera ), scaledData, scaleFactor, scaleFactor );
		pixel_count = (raspicam_wrapper_getHeight( Camera ) / scaleFactor) * (raspicam_wrapper_getWidth( Camera ) / scaleFactor);
		unsigned int heightStep = pixel_count / (raspicam_wrapper_getHeight( Camera ) / scaleFactor);
		unsigned int widthStep = (raspicam_wrapper_getWidth( Camera ) / scaleFactor) / 3;
		pixel_index = 0;
		unsigned int left_perimeter_average = 0;
		unsigned int right_perimeter_average = 0;
		unsigned int center_perimeter_average = 0;
		
		/*
		printf("\n");
		for(unsigned int x = 0; x < raspicam_wrapper_getHeight( Camera ) / scaleFactor; x++) {
			for(unsigned int y = 0; y < raspicam_wrapper_getWidth( Camera ) / scaleFactor; y++) {
				if(scaledData[(x * (raspicam_wrapper_getWidth( Camera ) / scaleFactor)) + y].R < 30) {
					printf("m");
				}
				else {
					printf("'");
				}
			}
			printf("\n");
		}
		*/
		 
		while (pixel_index <= pixel_count)
		{
			unsigned int width_pixel_index = 0;
			
			while(width_pixel_index <= widthStep) {
				left_perimeter_average += scaledData[pixel_index + width_pixel_index].R;
				center_perimeter_average += scaledData[pixel_index + width_pixel_index + widthStep].R;
				right_perimeter_average += scaledData[pixel_index + width_pixel_index + 2 * widthStep].R;
				
				width_pixel_index += 1;
			}
			
			pixel_index += heightStep;
		}
		
		left_perimeter_average = left_perimeter_average / ((pixel_count / 3) + 1);
		right_perimeter_average = right_perimeter_average / ((pixel_count / 3) + 1);
		center_perimeter_average = center_perimeter_average / ((pixel_count / 3) + 1);
		
		if(center_perimeter_average > right_perimeter_average && left_perimeter_average > right_perimeter_average) {
			returnValue = 1; // turn right
		}
		else if(center_perimeter_average > left_perimeter_average && right_perimeter_average > left_perimeter_average) {
			returnValue = 2; // turn left
		}
		else if(center_perimeter_average == left_perimeter_average && right_perimeter_average == left_perimeter_average && center_perimeter_average == right_perimeter_average) {
			returnValue = -1;
		}
		else {
			returnValue = 0; // do nothing	
		}
		
		
		/*printf("\n%d: Left: %d Center: %d Right: %d Returning: %d\n", count, left_perimeter_average, center_perimeter_average, right_perimeter_average, returnValue);
		Debug
		fprintf( outFile, "P6\n" );
		fprintf( outFile, "%d %d 255\n", raspicam_wrapper_getWidth( Camera ) / scaleFactor, raspicam_wrapper_getHeight( Camera ) / scaleFactor);
		fwrite( pixel, 1, raspicam_wrapper_getImageTypeSize( Camera, RASPICAM_WRAPPER_FORMAT_RGB ) / scaleFactor, outFile );
		printf( "Image saved at %s\n", buf );
		fclose( outFile );
		*/
		// free(scaledData);
	/*}
	else
	{
		printf( "unable to open output file\n" );
	}*/
	
	free(data);
	return returnValue; // Should come here
}

/*
 unsigned int scaleFactor = 20;
		struct pixel_format_RGB* scaledData = malloc(raspicam_wrapper_getImageTypeSize( Camera, RASPICAM_WRAPPER_FORMAT_RGB ) / scaleFactor);
		scale_image_data(pixel, raspicam_wrapper_getHeight( Camera ), raspicam_wrapper_getWidth( Camera ), scaledData, scaleFactor, scaleFactor );
		pixel_count = (raspicam_wrapper_getHeight( Camera ) / scaleFactor) * (raspicam_wrapper_getWidth( Camera ) / scaleFactor);
		unsigned char step = pixel_count / (raspicam_wrapper_getHeight( Camera ) / scaleFactor);
		pixel_index = 0;
		unsigned int left_perimeter_average = 0;
		unsigned int right_perimeter_average = 0;
		 
		while (pixel_index <= (raspicam_wrapper_getWidth( Camera ) / (scaleFactor * 4)))
		{
			
			scaledData[0 + pixel_index].R = 200;
			scaledData[0 + pixel_index].G = 0;
			scaledData[0 + pixel_index].B = 0;
			
			scaledData[(raspicam_wrapper_getWidth( Camera ) / scaleFactor) - 1 - pixel_index].R = 0;
			scaledData[(raspicam_wrapper_getWidth( Camera ) / scaleFactor) - 1 - pixel_index].G = 200;
			scaledData[(raspicam_wrapper_getWidth( Camera ) / scaleFactor) - 1 - pixel_index].B = 0;
			
			
			left_perimeter_average += scaledData[0 + pixel_index].R;
			right_perimeter_average += scaledData[(raspicam_wrapper_getWidth( Camera ) / scaleFactor) - 1 - pixel_index].R;
		
			pixel_index += 1;
		}
		
		//printf("\nLeft Average: %d Right Average: %d\n", left_perimeter_average, right_perimeter_average);
		left_perimeter_average = left_perimeter_average / (raspicam_wrapper_getWidth( Camera ) / (scaleFactor * 4));
		right_perimeter_average = right_perimeter_average / (raspicam_wrapper_getWidth( Camera ) / (scaleFactor * 4));
		
		if(left_perimeter_average < 30 && right_perimeter_average < 30) {
			returnValue = 0; // turn right
		}
		else if(right_perimeter_average < 30) {
			returnValue = 2; // turn left
		}
		else if(left_perimeter_average < 30) {
			returnValue = 1; // turn right
		}
		else {
			returnValue = 0; // do nothing	
		}
*/ 
