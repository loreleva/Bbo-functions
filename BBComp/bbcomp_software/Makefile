
SOURCES = bbcomplib.cpp problems.cpp rng.cpp json.cpp parser.cpp vector.cpp matrix.cpp paretofront.cpp hypervolume.cpp interpreter.cpp
OBJECTS = bbcomplib.o   problems.o   rng.o   json.o   parser.o   vector.o   matrix.o   paretofront.o   hypervolume.o   interpreter.o

# CC=gcc
# CXX=g++

example: libbbcomp.a example.c
	$(CXX) -o example example.c -L. -lbbcomp

libbbcomp.a: ${OBJECTS}
	ar rc libbbcomp.a ${OBJECTS}

%.o: %.c
	$(CC) -O3 -DNDEBUG -Wall -fPIC -c $< -o $@

%.o: %.cpp
	$(CXX) -std=c++11 -O3 -DNDEBUG -Wall -fPIC -c $< -o $@

clean:
	rm -f ${OBJECTS} libbbcomp.a example
