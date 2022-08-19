#UNCOMMENT AND INSERT BELOW THE NAME OF YOUR MAIN FILE
#MAIN_FILE = 
SOURCES = vector.cpp matrix.cpp interpreter.cpp parser.cpp
OBJECTS = vector.o   matrix.o   interpreter.o   parser.o

test: libbbcomp.a ${MAIN_FILE}
	$(CXX) -o main ${MAIN_FILE} -L. -lbbcomp

libbbcomp.a: ${OBJECTS}
	ar rc libbbcomp.a ${OBJECTS}

%.o: %.c
	$(CC) -O3 -DNDEBUG -Wall -fPIC -c $< -o $@

%.o: %.cpp
	$(CXX) -std=c++11 -O3 -DNDEBUG -Wall -fPIC -c $< -o $@

clean:
	rm -f ${OBJECTS} libbbcomp.a test
