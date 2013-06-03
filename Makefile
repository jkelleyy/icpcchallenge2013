
SRCS=$(wildcard ./*.cpp)
OBJS=$(SRCS:.cpp=.o)
CXXFLAGS=-Wall -Wextra -O


bistromathics : $(OBJS)
	g++ $(CXXFLAGS) $^ -lpthread -o $@

clean :
	rm -f *.o bistromathics
