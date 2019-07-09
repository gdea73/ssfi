TARGET = ./ssfi

OBJECTS = \
	search.o \
	ssfi.o \
	index.o

CXXFLAGS = -std=c++14 -Wall

LINKER_OPTIONS = -lpthread

all: $(OBJECTS)
	g++ -o $(TARGET) $(OBJECTS) $(LINKER_OPTIONS)

clean:
	rm -f ./*.o $(TARGET)
