objects := $(patsubst %.c,%.o,$(wildcard *.c))

all: myclient.out myserver.out

myclient.out: libstubs.a myclient.o
	gcc myclient.o -L. -lstubs -o myclient.out

myserver.out: libstubs.a myserver.o
	gcc myserver.o -L. -lstubs -o myserver.out

libstubs.a: server_stub.o client_stub.o mybind.o
	ar r libstubs.a server_stub.o client_stub.o mybind.o

$(objects): %.o: %.c ece454rpc_types.h
	gcc -c $< -o $@

clean:
	rm -rf *.out *.o core *.a
