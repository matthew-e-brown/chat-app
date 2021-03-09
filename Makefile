SERVER_MAIN := ./server/main.c
CLIENT_MAIN := ./client/main.c
WARN_ARGS := -Werror -Wall -Wextra

.PHONY: both
.PHONY: d-both

server.o:
	gcc $(SERVER_MAIN) $(WARN_ARGS) -o ./server.o -lcrypto -pthread

d-server.o:
	gcc $(SERVER_MAIN) $(WARN_ARGS) -o ./d-server.o -lcrypto -pthread -D__DEBUG__

client.o:
	gcc $(CLIENT_MAIN) $(WARN_ARGS) -o ./client.o -lcrypto -lncurses

both:
	rm ./server.o ./client.o -f && $(MAKE) server.o client.o

d-both:
	rm ./d-server.o ./client.o -f && $(MAKE) d-server.o client.o