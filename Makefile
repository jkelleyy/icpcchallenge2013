
SRCS=$(wildcard ./*.cpp)
OBJS=$(SRCS:.cpp=.o)


bistromathics : $(OBJS)
	g++ -Wall -O0 $^ -lpthread -o $@

clean :
	rm -f *.o bistromathics
