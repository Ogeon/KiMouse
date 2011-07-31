/*
 * imageOperations.h
 * Copyright (C) Erik Hedvall 2011 <admin@ogeon.se>
 *
 * kimouse is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * kimouse is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include <pthread.h>

#include <libfreenect.h>

#include <GL/glu.h>
#include <GL/glfw.h>

#include "imageOperations.h"

#define PI					3.1415
#define FREENECT_FRAME_PIX	640*480

freenect_context *f_ctx;
freenect_device *f_dev;

pthread_t processThread;
pthread_mutex_t gl_backbuf_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t gl_frame_cond = PTHREAD_COND_INITIALIZER;

uint16_t *depthBack, *depthFront, *depthReject;
uint8_t *depthRGBBack, *depthRGBFront;
char *depthBool;

GLuint glDepthTex;

int depthReceived;
int depthRGBReceived;

int running;
int cameraAngle;

int windowWidth, windowHeight;

int viewMode;


void resetBackground(){
	int i;
	for (i=0; i<FREENECT_FRAME_PIX; i++) {
		depthReject[i] = 0;
	}
}

void* processLoop(void* arg){
	while(running == 1 && freenect_process_events(f_ctx) >= 0){
		if(glfwGetKey('B')){
			resetBackground();
		}
		
		if(glfwGetKey('1')){
			viewMode = 0;
		}
		if(glfwGetKey('2')){
			viewMode = 1;
		}
		
		if(depthReceived){
			uint16_t* tmp = depthBack;
			depthBack = depthFront;
			depthFront = tmp;
			depthReceived = 0;
			
			laplace(depthFront, depthBool, 10);
			if(viewMode == 1){
				pthread_mutex_lock(&gl_backbuf_mutex);
				int i;
				for (i=0; i<FREENECT_FRAME_PIX; i++) {
					depthRGBBack[3*i+0] = depthBool[i];
					depthRGBBack[3*i+1] = depthBool[i];
					depthRGBBack[3*i+2] = depthBool[i];
				}
    			depthRGBReceived++;
    			pthread_cond_signal(&gl_frame_cond);
    			pthread_mutex_unlock(&gl_backbuf_mutex);
			}
		}
	}
	return NULL;
}

void depthCB(freenect_device *dev, void *v_depth, uint32_t timestamp){
	int i;
	//free(depthBack);
    depthBack = (uint16_t*)v_depth;
    if(viewMode == 0)
    	pthread_mutex_lock(&gl_backbuf_mutex);

    uint16_t d_min = 2047;
    uint16_t d_max = 0;
    
    for (i=0; i<FREENECT_FRAME_PIX; i++) {
        if(depthBack[i] < 2047){
            d_min = fmin(d_min, depthBack[i]);
            d_max = fmax(d_max, depthBack[i]);
        }
        
        if((depthReject[i] == 0 && depthBack[i] == 2047) ||
            (depthBack[i] > depthReject[i] && depthBack[i] != 2047)){
        	depthReject[i] = depthBack[i];
        }
    }

    for (i=0; i<FREENECT_FRAME_PIX; i++) {
        int lum = 0;
        if(depthBack[i] < 2047)
            lum = (int)(255-(depthBack[i] - d_min)/((float)d_max - d_min) * 255);
		
		if(viewMode == 0){
        	depthRGBBack[3*i+0] = lum;
        	depthRGBBack[3*i+1] = abs(depthBack[i] - depthReject[i]) > 10? lum: 0;
        	depthRGBBack[3*i+2] = abs(depthBack[i] - depthReject[i]) > 10? lum: 0;
        }
        
        if(abs(depthBack[i] - depthReject[i]) <= 10){
        	depthBack[i] = 2047;
        }
    }
    
    depthReceived++;
    
    if(viewMode == 0){
    	depthRGBReceived++;
    	pthread_cond_signal(&gl_frame_cond);
    	pthread_mutex_unlock(&gl_backbuf_mutex);
	}
}

void initGL(){
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0);
	glDepthFunc(GL_LESS);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glShadeModel(GL_SMOOTH);
	glGenTextures(1, &glDepthTex);
	glBindTexture(GL_TEXTURE_2D, glDepthTex);
	glEnable(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void resizeGL(int width, int height){
	if(height == 0) //Prevent divide by 0
		height = 1;
	if(width == 0)
		width = 1;
		
	if((float)width/(float)height > 640.0f/480.0f){
		windowWidth = ((float)width/(float)height) * 480;
		windowHeight = 480;
	}else{
		windowWidth = 640;
		windowHeight = ((float)height/(float)width) * 640;
	}
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, width, height);
	glOrtho (0, windowWidth, windowHeight, 0, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW); // Select The Modelview Matrix
	glLoadIdentity();
}

void redraw(){
	pthread_mutex_lock(&gl_backbuf_mutex);

	uint8_t *tmp;

	if (depthRGBReceived) {
		tmp = depthRGBFront;
		depthRGBFront = depthRGBBack;
		depthRGBBack = tmp;
		depthRGBReceived = 0;
	}
	
	pthread_mutex_unlock(&gl_backbuf_mutex);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glColor3f(1.0f, 1.0f, 1.0f);
	
	glBindTexture(GL_TEXTURE_2D, glDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, depthRGBFront);
	
	glBegin(GL_QUADS);
	glTexCoord2f(1, 0); glVertex2f(windowWidth/2 - 320, windowHeight/2 - 240);
	glTexCoord2f(0, 0); glVertex2f(windowWidth/2 + 320, windowHeight/2 - 240);
	glTexCoord2f(0, 1); glVertex2f(windowWidth/2 + 320, windowHeight/2 + 240);
	glTexCoord2f(1, 1); glVertex2f(windowWidth/2 - 320, windowHeight/2 + 240);
	glEnd();
	
	glBindTexture(GL_TEXTURE_2D, 0);
	//Draw tilt range background
	glColor3f(.3f, 1.0f, .3f);
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(10.0f, 40.0f);
	int angle;
	for(angle = -30; angle <= 30; angle+=15){
		glVertex2f(
		    10.0f + cos(angle/180.f * PI) * 50.0f,
		    40.0f + sin(angle/180.f * PI) * 50.0f);
	}
	glEnd();
	//Draw tilt range ticks
	glColor3f(.3f, 1.0f, .3f);
	glBegin(GL_LINES);
	glVertex2f(10.0f, 40.0f);
	glVertex2f(
	    10.0f + cos(30.f/180.f * PI) * 55.0f,
	    40.0f + sin(30.f/180.f * PI) * 55.0f);
	glVertex2f(10.0f, 40.0f);
	glVertex2f(
	    10.0f + cos(-30.f/180.f * PI) * 55.0f,
	    40.0f + sin(-30.f/180.f * PI) * 55.0f);
	glVertex2f(10.0f, 40.0f);
	glVertex2f(65.0f, 40.0f);
	glEnd();
	
	//Draw camera angle
	glColor3f(1.0f, .3f, .3f);
	glBegin(GL_LINES);
	glVertex2f(10.0f, 40.0f);
	glVertex2f(
	    10.0f + cos(cameraAngle/180.f * PI) * 50.0f,
	    40.0f - sin(cameraAngle/180.f * PI) * 50.0f);
	glEnd();
	glfwSwapBuffers();
}

static void tiltCamera(){
	static int angleCount = 0;
	if(++angleCount > 10){
		if(glfwGetKey(GLFW_KEY_UP) && cameraAngle < 30){
			cameraAngle ++;
		}
		if(glfwGetKey(GLFW_KEY_SPACE)){
			cameraAngle = 0;
		}
		if(glfwGetKey(GLFW_KEY_DOWN) && cameraAngle > -30){
			cameraAngle --;
		}
		freenect_set_tilt_degs(f_dev, cameraAngle);
		angleCount = 0;
	}
}

int main(int argc, char **argv){
	char verbose = 0;
	if(argc > 1){
		int i;
		for(i = 1; i < argc; i++){
			if(strcmp("--help", argv[i]) || strcmp("-h", argv[i])){
				printf("\n--KiMouse--\n");
				printf("Hand gesture based human-computer interface using a ");
				printf("Microsoft Kinect and libfreenect.\n\n");
				printf("Running the program:\n\tkimouse [flags]\n\n");
				printf("Flags:\n");
				printf("--help, -h\n\tShow this help text.\n\n");
				printf("--verbose, -v\n\tShow debug information from libfreenect.\n\n");
				exit(0);
			}
			if(strcmp("--verbose", argv[i]) || strcmp("-v", argv[i])){
				verbose = 1;
			}
		}
	}

	printf("Init freenect...");
	if (freenect_init(&f_ctx, NULL) < 0) {
		printf("\n\tfreenect_init() failed!\n");
		return 1;
	}
	
	if(verbose){
		freenect_set_log_level(f_ctx, FREENECT_LOG_DEBUG);
	}

	printf("\33[2K\rInit freenect done\n");

	printf("Searching for devices...");
	int nr_devices = freenect_num_devices (f_ctx);
	printf ("\33[2K\rNumber of devices found: %d\n",  nr_devices);

	int user_device_number = 0;

	if (nr_devices < 1){
		printf("Please connect a device before running KiMouse!\n");
		return 1;
	}

	printf("Connecting to device number %d... ", user_device_number+1);
	if (freenect_open_device(f_ctx, &f_dev, user_device_number) < 0) {
		printf("\rCould not open device number %d!\n", user_device_number+1);
		return 1;
	}
	printf("\33[2K\rDevice number %d is now connected\n", user_device_number+1);
	freenect_set_led(f_dev,LED_GREEN);
	freenect_set_tilt_degs(f_dev, -5);
	sleep(1);
	
	printf("\nHello!\n\n");
	freenect_set_tilt_degs(f_dev, 0);
	sleep(1);
	
	depthRGBReceived = 0;
	depthReceived = 0;
	viewMode = 0;
	
	depthRGBBack = (uint8_t*)malloc(FREENECT_FRAME_PIX * 3);
	depthRGBFront = (uint8_t*)malloc(FREENECT_FRAME_PIX * 3);
	depthReject = (uint16_t*)malloc(FREENECT_FRAME_PIX * sizeof(uint16_t));
	resetBackground();
	depthBool = (char*)malloc(FREENECT_FRAME_PIX);
	
	freenect_set_led(f_dev,LED_RED);
	freenect_set_depth_callback(f_dev, depthCB);
	freenect_set_depth_mode(f_dev,
	    freenect_find_depth_mode(
	        FREENECT_RESOLUTION_MEDIUM,
	        FREENECT_DEPTH_11BIT)
	    );
	freenect_start_depth(f_dev);
		
	running = 1;
	
	printf("Creating worker thread... ");
	int res = pthread_create(&processThread, NULL, processLoop, NULL);
	if (res) {
		printf("\n\tpthread_create failed!\n");
		return 1;
	}
	printf("\33[2K\rWorker thread created");
	
	printf("Initiating GLFW...");
	if (!glfwInit()) {
		printf("\n\tglfwInit failed!\n");
		return 1;
	}
	printf("\33[2K\rInit GLFW done");
	
	printf("Opening GLFW window...");
	if (!glfwOpenWindow(
	    640, 480,   // Resolution
	    8, 8, 8,	// Color buffer depth
	    8,		  // Alpha buffer depth
	    24,		  // Depth buffer
	    0,		  // Stencil buffer
	    GLFW_WINDOW)){
		printf("\n\tglfwOpenWindow failed!\n");
		return 1;
	}
	glfwSetWindowTitle("KiMouse");
	glfwSetWindowSizeCallback(resizeGL);
	printf("\33[2K\rGLFW window is open\n");
	
	initGL();
	
	cameraAngle = 0;
	while(running){
		redraw();
		tiltCamera();
		running = !glfwGetKey( GLFW_KEY_ESC ) &&
		    glfwGetWindowParam( GLFW_OPENED );
	}
	glfwTerminate();
	
	printf("\nShutting down...\n");
	fflush(stdout);

	printf("Reseting camera...\n");
	freenect_set_led(f_dev,LED_BLINK_GREEN);
	freenect_set_tilt_degs(f_dev, 0);

	printf("Halting depth capture...\n");
	freenect_stop_depth(f_dev);

	printf("Disconnecting camera...\n");
	freenect_close_device(f_dev);
	freenect_shutdown(f_ctx);
	
	printf("Freeing resources...\n");
	free(depthRGBBack);
	free(depthRGBFront);
	free(depthReject);
	free(depthBool);

	printf("Shutdown completed!\n\nKTHXBYE!\n");
	return 0;
}

