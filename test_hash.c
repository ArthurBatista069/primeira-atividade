/**
 * @file test_hash.c
 * @brief Testes unitarios para o modulo de Hash File Extensivel
 *
 * Compilar e executar:
 *   gcc -Wall -Wextra -o test_hash test_hash.c hash_extensivel.c
 *   ./test_hash
 *
 * Cada teste imprime [PASS] ou [FAIL] com descricao.
 */

#include "hash_extensivel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================
 * Infraestrutura de teste
 * ========================================================= */

static int total  = 0;
static int passou = 0;

#define ASSERT(descricao, condicao)                         \
    do {                                                    \
        total++;                                            \
        if (condicao) {                                     \
            printf("[PASS] %s\n", descricao);               \
            passou++;                                       \
        } else {                                            \
            printf("[FAIL] %s  (linha %d)\n",               \
                   descricao, __LINE__);                    \
        }                                                   \
    } while (0)

#define ARQUIVO_TESTE "teste_hash.dat"

static void limpar_arquivos(void) {
    remove(ARQUIVO_TESTE);
    remove(ARQUIVO_TESTE ".dir");
}

static Registro fazer_registro(int chave, const char *valor) {
    Registro r;
    r.chave = chave;
    strncpy(r.valor, valor, sizeof(r.valor) - 1);
    r.valor[sizeof(r.valor) - 1] = '\0';
    return r;
}

/* =========================================================
 * Grupo 1: hash_calcular_indice
 * ========================================================= */

static void teste_calcular_indice(void) {
    printf("\n--- Grupo 1: hash_calcular_indice ---\n");

    ASSERT("chave 0, pg=1 => indice 0", hash_calcular_indice(0, 1) == 0);
    ASSERT("chave 1, pg=1 => indice 1", hash_calcular_indice(1, 1) == 1);
    ASSERT("chave 2, pg=1 => indice 0", hash_calcular_indice(2, 1) == 0);
    ASSERT("chave 3, pg=1 => indice 1", hash_calcular_indice(3, 1) == 1);

    ASSERT("chave 4, pg=2 => indice 0", hash_calcular_indice(4, 2) == 0);
    ASSERT("chave 5, pg=2 => indice 1", hash_calcular_indice(5, 2) == 1);
    ASSERT("chave 6, pg=2 => indice 2", hash_calcular_indice(6, 2) == 2);
    ASSERT("chave 7, pg=2 => indice 3", hash_calcular_indice(7, 2) == 3);
}

/* =========================================================
 * Grupo 2: Criacao e fechamento
 * ========================================================= */

static void teste_criar_fechar(void) {
    printf("\n--- Grupo 2: criacao e fechamento ---\n");
    limpar_arquivos();

    HashExtensivel *h = hash_criar(ARQUIVO_TESTE);
    ASSERT("hash_criar retorna ponteiro nao-nulo", h != NULL);

    if (h) {
        ASSERT("profundidade global inicial = 1",
               h->diretorio.profundidade_global == 1);
        ASSERT("tamanho do diretorio inicial = 2",
               h->diretorio.tamanho == 2);
        hash_fechar(h);
    }

    h = hash_criar(ARQUIVO_TESTE);
    ASSERT("reabre arquivo existente sem erro", h != NULL);
    if (h) hash_fechar(h);

    limpar_arquivos();
}

/* =========================================================
 * Grupo 3: Insercao e busca
 * ========================================================= */

static void teste_inserir_buscar(void) {
    printf("\n--- Grupo 3: insercao e busca ---\n");
    limpar_arquivos();

    HashExtensivel *h = hash_criar(ARQUIVO_TESTE);
    if (!h) { printf("[FAIL] nao criou hash\n"); return; }

    Registro r1 = fazer_registro(10, "dez");
    Registro r2 = fazer_registro(20, "vinte");
    Registro r3 = fazer_registro(15, "quinze");

    ASSERT("inserir chave 10 retorna 0", hash_inserir(h, r1) == 0);
    ASSERT("inserir chave 20 retorna 0", hash_inserir(h, r2) == 0);
    ASSERT("inserir chave 15 retorna 0", hash_inserir(h, r3) == 0);

    Registro saida;

    ASSERT("busca chave 10 retorna 0",   hash_buscar(h, 10, &saida) == 0);
    ASSERT("valor de chave 10 correto",  strcmp(saida.valor, "dez") == 0);

    ASSERT("busca chave 20 retorna 0",   hash_buscar(h, 20, &saida) == 0);
    ASSERT("valor de chave 20 correto",  strcmp(saida.valor, "vinte") == 0);

    ASSERT("busca chave 15 retorna 0",   hash_buscar(h, 15, &saida) == 0);
    ASSERT("valor de chave 15 correto",  strcmp(saida.valor, "quinze") == 0);

    ASSERT("busca chave inexistente retorna -1",
           hash_buscar(h, 99, &saida) == -1);

    hash_fechar(h);
    limpar_arquivos();
}

/* =========================================================
 * Grupo 4: Chave duplicada
 * ========================================================= */

static void teste_duplicata(void) {
    printf("\n--- Grupo 4: chave duplicada ---\n");
    limpar_arquivos();

    HashExtensivel *h = hash_criar(ARQUIVO_TESTE);
    if (!h) return;

    Registro r = fazer_registro(42, "quarenta e dois");
    ASSERT("primeira insercao retorna 0", hash_inserir(h, r) == 0);
    ASSERT("segunda insercao retorna -1", hash_inserir(h, r) == -1);

    hash_fechar(h);
    limpar_arquivos();
}

/* =========================================================
 * Grupo 5: Remocao logica
 * ========================================================= */

static void teste_remover(void) {
    printf("\n--- Grupo 5: remocao ---\n");
    limpar_arquivos();

    HashExtensivel *h = hash_criar(ARQUIVO_TESTE);
    if (!h) return;

    Registro r = fazer_registro(7, "sete");
    hash_inserir(h, r);

    Registro saida;
    ASSERT("busca antes da remocao encontra",
           hash_buscar(h, 7, &saida) == 0);

    ASSERT("remover chave existente retorna 0",
           hash_remover(h, 7) == 0);

    ASSERT("busca apos remocao retorna -1",
           hash_buscar(h, 7, &saida) == -1);

    ASSERT("remover chave ja removida retorna -1",
           hash_remover(h, 7) == -1);

    ASSERT("remover chave inexistente retorna -1",
           hash_remover(h, 999) == -1);

    hash_fechar(h);
    limpar_arquivos();
}

/* =========================================================
 * Grupo 6: Split de bucket
 * ========================================================= */

static void teste_split(void) {
    printf("\n--- Grupo 6: split de bucket ---\n");
    limpar_arquivos();

    HashExtensivel *h = hash_criar(ARQUIVO_TESTE);
    if (!h) return;

    int chaves[] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20};
    int n = (int)(sizeof(chaves) / sizeof(chaves[0]));
    char buf[32];
    int ok = 1;

    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "val_%d", chaves[i]);
        Registro r = fazer_registro(chaves[i], buf);
        if (hash_inserir(h, r) != 0) {
            ok = 0;
            printf("  [FAIL] falha ao inserir chave %d\n", chaves[i]);
        }
    }
    ASSERT("insercao de 11 chaves pares (forcando splits) sem erro", ok);

    int todas_ok = 1;
    Registro saida;
    for (int i = 0; i < n; i++) {
        snprintf(buf, sizeof(buf), "val_%d", chaves[i]);
        if (hash_buscar(h, chaves[i], &saida) != 0 ||
            strcmp(saida.valor, buf) != 0) {
            todas_ok = 0;
            printf("  [FAIL] chave %d nao encontrada apos splits\n", chaves[i]);
        }
    }
    ASSERT("todas as chaves encontradas apos splits", todas_ok);

    ASSERT("profundidade global cresce apos splits",
           h->diretorio.profundidade_global > 1);

    hash_fechar(h);
    limpar_arquivos();
}

/* =========================================================
 * Grupo 7: Persistencia
 * ========================================================= */

static void teste_persistencia(void) {
    printf("\n--- Grupo 7: persistencia ---\n");
    limpar_arquivos();

    HashExtensivel *h = hash_criar(ARQUIVO_TESTE);
    if (!h) return;

    hash_inserir(h, fazer_registro(1, "um"));
    hash_inserir(h, fazer_registro(2, "dois"));
    hash_inserir(h, fazer_registro(3, "tres"));
    hash_fechar(h);

    h = hash_criar(ARQUIVO_TESTE);
    ASSERT("reabre arquivo apos insercoes", h != NULL);

    if (h) {
        Registro saida;
        ASSERT("chave 1 persiste apos reabrir",
               hash_buscar(h, 1, &saida) == 0 &&
               strcmp(saida.valor, "um") == 0);
        ASSERT("chave 2 persiste apos reabrir",
               hash_buscar(h, 2, &saida) == 0 &&
               strcmp(saida.valor, "dois") == 0);
        ASSERT("chave 3 persiste apos reabrir",
               hash_buscar(h, 3, &saida) == 0 &&
               strcmp(saida.valor, "tres") == 0);
        hash_fechar(h);
    }

    limpar_arquivos();
}

/* =========================================================
 * Main
 * ========================================================= */

int main(void) {
    printf("======================================\n");
    printf("  Testes - Hash File Extensivel\n");
    printf("======================================\n");

    teste_calcular_indice();
    teste_criar_fechar();
    teste_inserir_buscar();
    teste_duplicata();
    teste_remover();
    teste_split();
    teste_persistencia();

    printf("\n======================================\n");
    printf("Resultado: %d/%d testes passaram\n", passou, total);
    printf("======================================\n");

    return (passou == total) ? 0 : 1;
}
