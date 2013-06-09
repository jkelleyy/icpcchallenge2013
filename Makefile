
SRCS=$(wildcard ./*.cpp)
OBJS=$(SRCS:.cpp=.o)
CXXFLAGS=-lrt -pthread -O

DEPENDFILE=Makefile.depend

all : bistromathics

bistromathics : $(OBJS)
	g++ $(CXXFLAGS) $^ -lpthread -lrt -o $@

clean :
	rm -f *.o bistromathics
	rm -f $(DEPENDFILE)

depend : Makefile.depend

Makefile.depend :
	touch $(DEPENDFILE)
	makedepend -Y -f $(DEPENDFILE) -- $(CXXFLAGS) -- $(SRCS) >&/dev/null


include $(DEPENDFILE)
