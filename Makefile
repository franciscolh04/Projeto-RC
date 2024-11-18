CC = gcc
CFLAGS = -Wall -g

# Diretórios
SERVER_SRC = src/server/GS.c
PLAYER_SRC = src/client/player.c

# Executáveis
GS_EXEC = GS
PLAYER_EXEC = player

# Regras para compilar
all: $(GS_EXEC) $(PLAYER_EXEC)

# Compilar o GS
$(GS_EXEC): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(GS_EXEC) $(SERVER_SRC)

# Compilar o player
$(PLAYER_EXEC): $(PLAYER_SRC)
	$(CC) $(CFLAGS) -o $(PLAYER_EXEC) $(PLAYER_SRC)

# Limpeza dos arquivos gerados
clean:
	rm -f $(GS_EXEC) $(PLAYER_EXEC)
