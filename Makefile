# ============================================================
# Makefile - Hash File Extensível
# Uso:
#   make          -> compila tudo
#   make test     -> compila e roda os testes
#   make clean    -> remove arquivos gerados
# ============================================================
# pra rodar gcc -Wall -Wextra -o test_hash test_hash.c hash_extensivel.c 
#./test_hash
CC      = gcc
CFLAGS  = -Wall -Wextra -g

# Arquivos fonte
SRC     = hash_extensivel.c
HDR     = hash_extensivel.h
TEST    = test_hash.c

# Nome do executável de teste
TARGET  = test_hash

# Regra padrão
all: $(TARGET)

# Compila o executável de testes
$(TARGET): $(TEST) $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $(TARGET) $(TEST) $(SRC)

# Compila e executa os testes
test: $(TARGET)
	./$(TARGET)

# Remove arquivos gerados
clean:
	rm -f $(TARGET) *.dat *.dir

.PHONY: all test clean
