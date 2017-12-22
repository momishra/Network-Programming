CC = gcc

LIBS = -lpthread -lm -lc \
        /home/courses/cse533/Stevens/unpv13e_linux/libunp.a

FLAGS = -g3
CFLAGS = ${FLAGS} -I/home/courses/cse533/Stevens/unpv13e_linux/lib

all: momishra_tour momishra_arp

momishra_tour: get_hw_addrs.o tour.o   
	${CC} ${FLAGS} -o momishra_tour tour.o get_hw_addrs.o  ${LIBS}
tour.o: tour.c
	${CC} ${CFLAGS} -c tour.c

momishra_arp: get_hw_addrs.o arp.o
	${CC} ${FLAGS} -o momishra_arp arp.o get_hw_addrs.o ${LIBS}
arp.o: arp.c
	${CC} ${CFLAGS} -c arp.c

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${CFLAGS} -c get_hw_addrs.c

clean:
	rm -rf momishra_tour tour.o get_hw_addrs.o arp.o momishra_arp
