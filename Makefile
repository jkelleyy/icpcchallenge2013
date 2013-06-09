
SRCS=$(wildcard ./*.cpp)
OBJS=$(SRCS:.cpp=.o)
CXXFLAGS=-Wall -Wextra -O -g -DDEBUG_OUTPUT_ON=1

DEPENDFILE=Makefile.depend

all : bistromathics

bistromathics : $(OBJS)
	g++ $(CXXFLAGS) $^ -lpthread -o $@

clean :
	rm -f *.o bistromathics
	rm -f $(DEPENDFILE)

depend : Makefile.depend

Makefile.depend :
	touch $(DEPENDFILE)
	makedepend -Y -f $(DEPENDFILE) -- $(CXXFLAGS) -- $(SRCS) >&/dev/null


include $(DEPENDFILE)
