CC = gcc
INC = ../inc
BIN = ../bin

OD = ../obj

ODCL = $(OD)/client
ODSE = $(OD)/server
ODSH = $(OD)/shared

CLIENT_OBJS = $(ODCL)/main.o $(ODCL)/game.o
SERVER_OBJS =
SHARED_OBJS = $(ODSH)/flecs.o

CFLAGS = -std=gnu99 -O2 -g -c -Wall -I $(INC)
CLIENT_LDFLAGS = -lglfw -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lraylib -lm


$(ODCL)/%.o: client/%.c
	@mkdir -p $(ODCL)
	$(CC) $(CFLAGS) $< -o $@

$(ODSE)/%.o: server/%.c
	@mkdir -p $(ODSE)
	$(CC) $(CFLAGS) $< -o $@

$(ODSH)/%.o: shared/%.c
	@mkdir -p $(ODSH)
	$(CC) $(CFLAGS) $< -o $@


client: $(CLIENT_OBJS) $(SHARED_OBJS)
	@mkdir -p $(BIN)
	$(CC) $(CLIENT_OBJS) $(SHARED_OBJS) -o $(BIN)/client $(CLIENT_LDFLAGS)


server: $(SERVER_OBJS) $(SHARED_OBJS)
	@mkdir -p $(BIN)
	$(CC) $(SERVER_OBJS) $(SHARED_OBJS) -o $(BIN)/server

$(ODCL)/main.o:

$(ODCL)/game.o: $(INC)/game.h

$(ODSH)/flecs.o: $(INC)/flecs.h
