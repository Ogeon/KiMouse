OBJECTS = build/main.o
CC = gcc
INCLUDES = -Ilibs/libfreenect/include

#### FLAGS ####
#Allways provided:
CFLAGS = -Wall

#Provided when building .o-files:
OFLAGS = -c

#Provided when building executable:
EFLAGS =

#### BUILD COMMANDS ####
#Build .o-file
OBUILD = $(CC) $(CFLAGS) $(OFLAGS) $(INCLUDES)
#Build binary
BBUILD = $(CC) $(CFLAGS) $(EFLAGS) $(LINKS) $(OBJECTS)


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