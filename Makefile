OBJECTS = build/main.o build/imageOperations.o build/integerList.o
CC = gcc
INCLUDES = -Ilibs/libfreenect/include -Ilibs/glfw-2.7/include
LINKS = -lfreenect -lGL -lGLU -lm libs/glfw-2.7/lib/x11/libglfw.a -lXrandr

#### FLAGS ####
#Allways provided:
CFLAGS = -Wall

#Provided when building .o-files:
OFLAGS = -c

#Provided when building executable:
EFLAGS =

#### BUILD COMMANDS ####
#Build .o-file:
OBUILD = $(CC) $(CFLAGS) $(OFLAGS) $(INCLUDES)
#Build binary:
BBUILD = $(CC) $(CFLAGS) $(EFLAGS) $(OBJECTS) $(LINKS)


#### TARGETS ####
#Phony:
all: bin/kimouse

clean:
	rm -f -r build

clean-all: clean
	rm -f -r bin

.PHONY: all clean clean-all


#Build:
bin/kimouse: build bin $(OBJECTS)
	$(BBUILD) -o bin/kimouse

build:
	mkdir build

bin:
	mkdir bin

build/main.o: src/main.c
	$(OBUILD) src/main.c -o build/main.o
	
build/imageOperations.o: src/imageOperations.c src/imageOperations.h
	$(OBUILD) src/imageOperations.c -o build/imageOperations.o
	
build/integerList.o: src/integerList.c src/integerList.h
	$(OBUILD) src/integerList.c -o build/integerList.o
