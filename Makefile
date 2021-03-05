server.o:
	gcc ./server/main.c -Werror -Wall -Wextra -o ./server.o -lcrypto -pthread

client.o:
	gcc ./client/main.c -Werror -Wall -Wextra -o ./client.o -lcrypto -lncurses

.PHONY: both
both:
	rm ./server.o ./client.o -f && $(MAKE) server.o client.o