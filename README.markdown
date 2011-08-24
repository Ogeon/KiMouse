#KiMouse

##About
KiMouse is a terrible name of a (hopefully) usable interface between a human user and his/her computer. The name comes from its main purpose: Using a Microsoft Kinect to control/substitute the computer mouse using hand gestures.

##Building
It's easy to build kimouse if you have the right setup. There is a Makefile doing the horrible linking and file building job for you, so the only thing you have to do before building it is to make sure you have OpenGL, pthreads and a precompiled and working copy of libfreenect installed in your system and a precompiled and working copy of GLFW 7.2 in a directory called "libs" outside "src".

Now, when you have all the files and packages at the right place, you just have to run

	$ make

or (same thing)

	$ make all

here in the project root and it will build.

##Installing
No, you don't need to install it. Why would you?

##Running
Just run the file `bin/kimouse`. That's all!
