# compile the library and copy to destination directory

.PHONY : clean

CC = arm-linux-gnueabihf-gcc

CFLAGS= -g -c -fPIC -Wall
LIBFLAGS= -g -shared

SOURCES = rowAndColumnDriver.c
OBJECTS = rowAndColumnDriver.o
TARGET = libRowAndColDriver.so

all: $(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)
	
$(OBJECTS) : $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -lm

$(TARGET) : $(OBJECTS)
	$(CC) $(LIBFLAGS) -o $(TARGET) $(OBJECTS)

install:
	mkdir -p $(DESTDIR)/opt/kymeta/lib
	cp $(TARGET) /opt/kymeta/lib