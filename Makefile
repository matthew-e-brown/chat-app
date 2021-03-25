WARN_ARGS    := -Werror -Wall -Wextra
SERVER_FILES := $(shell find server -name "*.c" ! -name "main.c")
CLIENT_FILES := $(shell find client -name "*.c" ! -name "main.c")
SHARED_FILES := $(shell find shared -name "*.c")

.PHONY: both
.PHONY: d-both

server.o:
	gcc "./server/main.c" $(SHARED_FILES) $(SERVER_FILES) $(WARN_ARGS) \
		-o ./server.o \
		-lcrypto -pthread

d-server.o:
	gcc "./server/main.c" $(SHARED_FILES) $(SERVER_FILES) $(WARN_ARGS) \
		-o ./d-server.o \
		-lcrypto -pthread \
		-D__DEBUG__

client.o:
	gcc "./client/main.c" $(SHARED_FILES) $(CLIENT_FILES) $(WARN_ARGS) \
		-o ./client.o\
		-lcrypto -lncurses

both:
	rm ./server.o ./client.o -f && $(MAKE) server.o client.o

d-both:
	rm ./d-server.o ./client.o -f && $(MAKE) d-server.o client.o