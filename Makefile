# Definir variáveis
CC = gcc
CFLAGS = -Wall -g  # Flags de compilação (warnings e depuração)
SRC_DIR = src/server  # Diretório onde está o código fonte
OUT_DIR = .  # Diretório de saída (onde o executável será criado)
OUT_NAME = GS  # Nome do executável
SRC_FILE = $(SRC_DIR)/GS.c  # Caminho para o ficheiro de código fonte

# Alvo padrão: compilar o programa
all: $(OUT_NAME)

# Como compilar o executável
$(OUT_NAME): $(SRC_FILE)
	$(CC) $(CFLAGS) $(SRC_FILE) -o $(OUT_NAME)

# Limpar ficheiros gerados (executáveis, etc.)
clean:
	rm -f $(OUT_NAME)

.PHONY: all clean
