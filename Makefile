WARN_ARGS    := -Werror -Wall -Wextra
SERVER_FILES := $(shell find server -name "*.c" ! -name "main.c")
CLIENT_FILES := $(shell find client -name "*.c" ! -name "main.c")
SHARED_FILES := $(shell find shared -name "*.c")

.PHONY: both
.PHONY: d-both
.PHONY: clean

server.o:
	gcc "./server/main.c" $(SHARED_FILES) $(SERVER_FILES) $(WARN_ARGS) -lcrypto -pthread -o ./server.o

d-server.o:
	gcc "./server/main.c" $(SHARED_FILES) $(SERVER_FILES) $(WARN_ARGS) -lcrypto -pthread -D__DEBUG__ -o ./d-server.o

client.o:
	gcc "./client/main.c" $(SHARED_FILES) $(CLIENT_FILES) $(WARN_ARGS) -lcrypto -lncurses -o ./client.o

both:
	rm ./server.o ./client.o -f && $(MAKE) server.o client.o

d-both:
	rm ./d-server.o ./client.o -f && $(MAKE) d-server.o client.o

clean:
	rm ./server.o ./d-server.o ./client.o -f