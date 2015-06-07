FLAGS=-Wall -std=gnu99
OPTIMISATIONS=-O3 -flto
CFLAGS=`pkg-config --cflags gio-2.0 x11`
LIBS=`pkg-config --libs gio-2.0 x11`
SOURCE=mousewheelzoom.c
OBJECT=mousewheelzoom.o
TARGET=mousewheelzoom

$(TARGET): $(OBJECT)
	$(CC) $(FLAGS) $(OPTIMISATIONS) -s -o $(TARGET) $(OBJECT) $(LIBS)

$(OBJECT): $(SOURCE)
	$(CC) $(FLAGS) $(OPTIMISATIONS) -c -o $(OBJECT) $(SOURCE) $(CFLAGS)

all: $(TARGET)

again: clean all

clean:
	rm -f $(OBJECT) $(TARGET)
