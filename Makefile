COMPILE_ARGS := -Werror -Wall -Wextra -std=c99 -pedantic
SERVER_FILES := $(shell find server -name "*.c" ! -name "main.c")
CLIENT_FILES := $(shell find client -name "*.c" ! -name "main.c")
SHARED_FILES := $(shell find shared -name "*.c")

.PHONY: both
.PHONY: d-both
.PHONY: clean

both:
	rm server.o client.o -f && $(MAKE) server.o client.o

d-both:
	rm d-server.o client.o -f && $(MAKE) d-server.o client.o


server.o:
	gcc "server/main.c" $(SHARED_FILES) $(SERVER_FILES) -lcrypto -pthread -o server.o $(COMPILE_ARGS)

d-server.o:
	gcc "server/main.c" $(SHARED_FILES) $(SERVER_FILES) -lcrypto -pthread -D__DEBUG__ -o d-server.o $(COMPILE_ARGS)


client.o:
	gcc "client/main.c" $(SHARED_FILES) $(CLIENT_FILES) -lcrypto -lncurses -o client.o $(COMPILE_ARGS)


clean:
	rm server.o d-server.o client.o -f