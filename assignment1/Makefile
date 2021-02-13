all: client.o server.o

client.o:
	gcc ./client/main.c -Werror -Wall -Wextra -o ./client.o -pthread -lncurses

server.o:
	gcc ./server/main.c -Werror -Wall -Wextra -o ./server.o -pthread

clean:
	rm ./server.o ./client.o