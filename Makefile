server.o:
	gcc ./server/main.c -Werror -Wall -Wextra -o ./server.o -lcrypto -pthread