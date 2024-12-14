CC = gcc
CFLAGS = -Wall -g

# Diretórios
SERVER_SRC = src/server
CLIENT_SRC = src/client

# Ficheiros do servidor
SERVER_OBJS = $(SERVER_SRC)/command_handler.o $(SERVER_SRC)/game.o $(SERVER_SRC)/state.o $(SERVER_SRC)/GS.o

# Ficheiro do cliente
PLAYER_OBJS = $(CLIENT_SRC)/player.o

# Executáveis
GS_EXEC = GS
PLAYER_EXEC = player

# Regra padrão
all: $(GS_EXEC) $(PLAYER_EXEC)

# Compilar o GS
$(GS_EXEC): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(GS_EXEC) $(SERVER_OBJS) -lm

# Compilar o player
$(PLAYER_EXEC): $(PLAYER_OBJS)
	$(CC) $(CFLAGS) -o $(PLAYER_EXEC) $(PLAYER_OBJS)

# Compilar ficheiros individuais
$(SERVER_SRC)/command_handler.o: $(SERVER_SRC)/command_handler.c $(SERVER_SRC)/command_handler.h
	$(CC) $(CFLAGS) -c $(SERVER_SRC)/command_handler.c -o $(SERVER_SRC)/command_handler.o

$(SERVER_SRC)/game.o: $(SERVER_SRC)/game.c $(SERVER_SRC)/game.h
	$(CC) $(CFLAGS) -c $(SERVER_SRC)/game.c -o $(SERVER_SRC)/game.o

$(SERVER_SRC)/state.o: $(SERVER_SRC)/state.c $(SERVER_SRC)/state.h
	$(CC) $(CFLAGS) -c $(SERVER_SRC)/state.c -o $(SERVER_SRC)/state.o

$(SERVER_SRC)/GS.o: $(SERVER_SRC)/GS.c $(SERVER_SRC)/command_handler.h $(SERVER_SRC)/state.h $(SERVER_SRC)/game.h
	$(CC) $(CFLAGS) -c $(SERVER_SRC)/GS.c -o $(SERVER_SRC)/GS.o

$(CLIENT_SRC)/player.o: $(CLIENT_SRC)/player.c
	$(CC) $(CFLAGS) -c $(CLIENT_SRC)/player.c -o $(CLIENT_SRC)/player.o

# Limpeza dos arquivos gerados
clean:
	rm -f $(SERVER_SRC)/*.o $(CLIENT_SRC)/*.o $(GS_EXEC) $(PLAYER_EXEC)
