CXXFLAGS = -g -std=c++20
INCLUDE_PATHS = -Iraylib/src -Iraylib/src/external
LDFLAGS = -L./
LDLIBS = -lraylib -lGLESv2 -lEGL -lgbm -ldrm -lddcutil
SRCS = main.cpp framebuffercontainer.cpp taskHeap.cpp
OBJ = $(SRCS:.cpp=.o)
PROG = clock

all : $(PROG)

$(PROG) : $(OBJ)
	g++ -o $(PROG) $(OBJ) $(CXXFLAGS) $(INCLUDE_PATHS) $(LDFLAGS) $(LDLIBS)
	
clean:
	rm -f *.o $(PROG)