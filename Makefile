TOPDIR=..

LIBS+= -pthread

TARGET="proxy"


all: proxy.o fd.o protocol.o ev_epoll.o
	cc -lstdc++ -O -g -Wall -gdwarf-2 -std=c++11 -pthread -I../lib -o ${TARGET} proxy.o fd.o protocol.o ev_epoll.o ${LIBS}

ev_epoll.o : ev_epoll.h ev_epoll.cpp
	cc -lstdc++ -O -g -Wall -gdwarf-2 -std=c++11 -pthread -I../lib -c  ev_epoll.cpp ${LIBS}
fd.o : fd.h fd.cpp
	cc -lstdc++ -O -g -Wall -gdwarf-2 -std=c++11 -pthread -I../lib -c  -pthread fd.cpp ${LIBS}
protocol.o : protocol.h protocol.cpp 
	cc -lstdc++ -O -g -Wall -gdwarf-2 -std=c++11 -pthread -I../lib -c  -pthread protocol.cpp ${LIBS}
proxy.o : proxy.cpp config.hpp
	cc -lstdc++ -O -g -Wall -gdwarf-2 -std=c++11 -pthread -I../lib -c  proxy.cpp ${LIBS}

.PHONY: clean
clean:
	rm -f *.o ${TARGET}
