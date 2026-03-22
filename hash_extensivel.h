/**
 * @file hash_extensivel.h
 * @brief Modulo para manipulacao de Hash File Extensivel (Extendible Hashing)
 *
 * Hash Extensivel e uma estrutura de dados dinamica que cresce conforme
 * necessario, sem precisar rehashear toda a tabela. Utiliza um diretorio
 * de ponteiros para buckets e profundidades locais/globais para controlar
 * o crescimento.
 *
 * Conceitos principais:
 *  - Profundidade Global (pg): numero de bits usados pelo diretorio
 *  - Profundidade Local  (pl): numero de bits usados por cada bucket
 *  - Bucket: pagina de disco que armazena registros
 *
 * @author  (seu nome)
 * @date    2026
 */

#ifndef HASH_EXTENSIVEL_H
#define HASH_EXTENSIVEL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* =========================================================
 * Constantes configuráveis
 * ========================================================= */

/** Capacidade maxima de registros por bucket */
#define BUCKET_CAPACIDADE 4

/** Valor sentinela para chave invalida/removida */
#define CHAVE_INVALIDA -1

/* =========================================================
 * Estruturas de dados
 * ========================================================= */

/**
 * @brief Registro armazenado no hash file.
 * Adapte os campos conforme o tipo de dado da sua aplicacao.
 */
typedef struct {
    int  chave;        /**< Chave de identificacao unica */
    char valor[64];    /**< Valor/dado associado a chave */
} Registro;

/**
 * @brief Bucket (balde) - unidade de armazenamento em disco.
 *
 * Cada bucket possui sua propria profundidade local e um vetor
 * fixo de registros.
 */
typedef struct {
    int      profundidade_local;           /**< Profundidade local do bucket */
    int      ocupacao;                     /**< Numero de registros no bucket */
    Registro registros[BUCKET_CAPACIDADE]; /**< Vetor de registros */
} Bucket;

/**
 * @brief Diretorio do hash extensivel.
 *
 * Contem 2^profundidade_global entradas, cada uma apontando para
 * o offset de um bucket no arquivo.
 */
typedef struct {
    int   profundidade_global; /**< Profundidade global atual */
    int   tamanho;             /**< Numero de entradas = 2^pg */
    long *offsets;             /**< Vetor de offsets dos buckets no arquivo */
} Diretorio;

/**
 * @brief Handle principal do hash extensivel.
 */
typedef struct {
    Diretorio diretorio;          /**< Diretorio em memoria */
    FILE     *arquivo;            /**< Ponteiro para o arquivo de dados */
    char      nome_arquivo[256];  /**< Nome do arquivo de dados */
} HashExtensivel;

/* =========================================================
 * Interface publica
 * ========================================================= */

/**
 * @brief Cria (ou abre) um hash file extensivel.
 *
 * Se o arquivo nao existir, e inicializado com profundidade global 1
 * e dois buckets vazios. Caso exista, carrega o diretorio salvo.
 *
 * @param nome_arquivo  Caminho do arquivo de dados (.dat)
 * @return Ponteiro para HashExtensivel alocado, ou NULL em erro.
 */
HashExtensivel *hash_criar(const char *nome_arquivo);

/**
 * @brief Fecha o hash file, salvando o diretorio e liberando memoria.
 * @param h  Ponteiro para o HashExtensivel.
 */
void hash_fechar(HashExtensivel *h);

/**
 * @brief Insere um registro no hash extensivel.
 *
 * Se o bucket destino estiver cheio, realiza split e, se necessario,
 * dobra o diretorio.
 *
 * @param h    Ponteiro para o HashExtensivel.
 * @param reg  Registro a ser inserido.
 * @return  0 em sucesso, -1 se chave duplicada, -2 em erro interno.
 */
int hash_inserir(HashExtensivel *h, Registro reg);

/**
 * @brief Busca um registro pela chave.
 *
 * @param h      Ponteiro para o HashExtensivel.
 * @param chave  Chave buscada.
 * @param saida  Onde o registro encontrado sera copiado.
 * @return  0 em sucesso, -1 se nao encontrado.
 */
int hash_buscar(HashExtensivel *h, int chave, Registro *saida);

/**
 * @brief Remove um registro pela chave (marcacao logica).
 *
 * O slot e marcado com CHAVE_INVALIDA. O bucket nao e fundido
 * automaticamente.
 *
 * @param h      Ponteiro para o HashExtensivel.
 * @param chave  Chave a remover.
 * @return  0 em sucesso, -1 se nao encontrado.
 */
int hash_remover(HashExtensivel *h, int chave);

/**
 * @brief Imprime o estado do diretorio e dos buckets (debug).
 * @param h  Ponteiro para o HashExtensivel.
 */
void hash_imprimir(HashExtensivel *h);

/* =========================================================
 * Funcoes auxiliares (internas, expostas para os testes)
 * ========================================================= */

/**
 * @brief Calcula o indice do diretorio para uma chave.
 *
 * Usa os 'pg' bits menos significativos da chave como indice.
 *
 * @param chave  Chave a mapear.
 * @param pg     Profundidade global.
 * @return Indice no diretorio [0, 2^pg).
 */
int hash_calcular_indice(int chave, int pg);

/**
 * @brief Le um bucket do arquivo a partir de um offset.
 * @param arquivo  Arquivo aberto.
 * @param offset   Posicao em bytes no arquivo.
 * @param bucket   Destino da leitura.
 * @return  0 em sucesso, -1 em erro.
 */
int bucket_ler(FILE *arquivo, long offset, Bucket *bucket);

/**
 * @brief Escreve um bucket no arquivo em um offset.
 * @param arquivo  Arquivo aberto.
 * @param offset   Posicao em bytes no arquivo.
 * @param bucket   Bucket a escrever.
 * @return  0 em sucesso, -1 em erro.
 */
int bucket_escrever(FILE *arquivo, long offset, const Bucket *bucket);

#endif /* HASH_EXTENSIVEL_H */
