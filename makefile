all: webserv client

clean:
	rm -rf webserv client webserv.o client.o

webserv:
	gcc webserv.c -g -o webserv

client:
	gcc client.c -g -o client