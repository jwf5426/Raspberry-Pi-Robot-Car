#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "camera_project/raspicam_wrapper.h"
#include "LowerResolution/pixel_format_RGB.h"
#include "LowerResolution/scale_image_data.h"

struct raspicam_wrapper_handle *  Camera;       /* Camera handle */

int initCamera();
int destoryCamera();
int captureCamera();
