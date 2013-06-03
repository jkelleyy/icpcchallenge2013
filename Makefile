
SRCS=$(wildcard ./*.cpp)
OBJS=$(SRCS:.cpp=.o)
CXXFLAGS=-Wall -Wextra -O

DEPENDFILE=Makefile.depend

all : bistromathics

bistromathics : $(OBJS)
	g++ $(CXXFLAGS) $^ -lpthread -o $@

clean :
	rm -f *.o bistromathics
	rm -f $(DEPENDFILE)

depend :
	touch $(DEPENDFILE)
	makedepend -Y -f $(DEPENDFILE) -- $(CXXFLAGS) -- $(SRCS) >&/dev/null


include $(DEPENDFILE)
