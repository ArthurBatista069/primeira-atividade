/**
 * @file hash_extensivel.c
 * @brief Implementacao do modulo de Hash File Extensivel
 *
 * Estrutura do arquivo de dados (.dat):
 *   - Os buckets sao gravados sequencialmente a partir do byte 0.
 *   - O diretorio e salvo no arquivo .dir (mesmo nome base).
 *   - Cada bucket ocupa sizeof(Bucket) bytes fixos.
 *
 * Algoritmo de insercao:
 *   1. Calcular indice = chave & (2^pg - 1)
 *   2. Ler o bucket apontado por diretorio.offsets[indice]
 *   3. Se bucket nao cheio: inserir e gravar
 *   4. Se cheio e pl < pg: split (dividir bucket)
 *   5. Se cheio e pl == pg: dobrar diretorio, depois split
 */

#include "hash_extensivel.h"

/* =========================================================
 * Utilitarios internos
 * ========================================================= */

/** Retorna 2^n */
static int potencia2(int n) {
    return 1 << n;
}

/* =========================================================
 * Operacoes de bucket em arquivo
 * ========================================================= */

int bucket_ler(FILE *arquivo, long offset, Bucket *bucket) {
    if (fseek(arquivo, offset, SEEK_SET) != 0) return -1;
    if (fread(bucket, sizeof(Bucket), 1, arquivo) != 1) return -1;
    return 0;
}

int bucket_escrever(FILE *arquivo, long offset, const Bucket *bucket) {
    if (fseek(arquivo, offset, SEEK_SET) != 0) return -1;
    if (fwrite(bucket, sizeof(Bucket), 1, arquivo) != 1) return -1;
    fflush(arquivo);
    return 0;
}

/**
 * @brief Adiciona um novo bucket vazio ao final do arquivo.
 * @return Offset do novo bucket, ou -1 em erro.
 */
static long bucket_novo(FILE *arquivo, int profundidade_local) {
    if (fseek(arquivo, 0, SEEK_END) != 0) return -1;
    long offset = ftell(arquivo);
    if (offset < 0) return -1;

    Bucket b;
    b.profundidade_local = profundidade_local;
    b.ocupacao = 0;

    for (int i = 0; i < BUCKET_CAPACIDADE; i++) {
        b.registros[i].chave = CHAVE_INVALIDA;
        memset(b.registros[i].valor, 0, sizeof(b.registros[i].valor));
    }

    if (fwrite(&b, sizeof(Bucket), 1, arquivo) != 1) return -1;
    fflush(arquivo);
    return offset;
}

/* =========================================================
 * Gerenciamento do diretorio
 * ========================================================= */

/**
 * @brief Dobra o diretorio (duplica o numero de entradas).
 *
 * Cada entrada nova aponta para o mesmo bucket da entrada antiga.
 * A profundidade global e incrementada em 1.
 */
static int diretorio_dobrar(Diretorio *d) {
    int novo_tamanho = d->tamanho * 2;
    long *novo_offsets = (long *)realloc(d->offsets, novo_tamanho * sizeof(long));
    if (!novo_offsets) return -1;

    for (int i = 0; i < d->tamanho; i++) {
        novo_offsets[d->tamanho + i] = novo_offsets[i];
    }

    d->offsets = novo_offsets;
    d->tamanho = novo_tamanho;
    d->profundidade_global++;
    return 0;
}

/* =========================================================
 * Criacao / abertura do hash
 * ========================================================= */

HashExtensivel *hash_criar(const char *nome_arquivo) {
    HashExtensivel *h = (HashExtensivel *)malloc(sizeof(HashExtensivel));
    if (!h) return NULL;

    strncpy(h->nome_arquivo, nome_arquivo, sizeof(h->nome_arquivo) - 1);
    h->nome_arquivo[sizeof(h->nome_arquivo) - 1] = '\0';

    h->arquivo = fopen(nome_arquivo, "r+b");

    if (h->arquivo == NULL) {
        /* Arquivo novo: inicializa com pg=1, dois buckets vazios */
        h->arquivo = fopen(nome_arquivo, "w+b");
        if (!h->arquivo) {
            free(h);
            return NULL;
        }

        h->diretorio.profundidade_global = 1;
        h->diretorio.tamanho = 2;
        h->diretorio.offsets = (long *)malloc(2 * sizeof(long));
        if (!h->diretorio.offsets) {
            fclose(h->arquivo);
            free(h);
            return NULL;
        }

        h->diretorio.offsets[0] = bucket_novo(h->arquivo, 1);
        h->diretorio.offsets[1] = bucket_novo(h->arquivo, 1);

        if (h->diretorio.offsets[0] < 0 || h->diretorio.offsets[1] < 0) {
            fclose(h->arquivo);
            free(h->diretorio.offsets);
            free(h);
            return NULL;
        }
    } else {
        /* Arquivo existente: le o diretorio do arquivo .dir */
        char nome_dir[260];
        snprintf(nome_dir, sizeof(nome_dir), "%s.dir", nome_arquivo);
        FILE *fdir = fopen(nome_dir, "rb");
        if (!fdir) {
            h->diretorio.profundidade_global = 1;
            h->diretorio.tamanho = 2;
            h->diretorio.offsets = (long *)malloc(2 * sizeof(long));
            h->diretorio.offsets[0] = 0;
            h->diretorio.offsets[1] = sizeof(Bucket);
        } else {
            fread(&h->diretorio.profundidade_global, sizeof(int), 1, fdir);
            fread(&h->diretorio.tamanho, sizeof(int), 1, fdir);
            h->diretorio.offsets = (long *)malloc(h->diretorio.tamanho * sizeof(long));
            fread(h->diretorio.offsets, sizeof(long), h->diretorio.tamanho, fdir);
            fclose(fdir);
        }
    }

    return h;
}

void hash_fechar(HashExtensivel *h) {
    if (!h) return;

    char nome_dir[260];
    snprintf(nome_dir, sizeof(nome_dir), "%s.dir", h->nome_arquivo);
    FILE *fdir = fopen(nome_dir, "wb");
    if (fdir) {
        fwrite(&h->diretorio.profundidade_global, sizeof(int), 1, fdir);
        fwrite(&h->diretorio.tamanho, sizeof(int), 1, fdir);
        fwrite(h->diretorio.offsets, sizeof(long), h->diretorio.tamanho, fdir);
        fclose(fdir);
    }

    free(h->diretorio.offsets);
    fclose(h->arquivo);
    free(h);
}

/* =========================================================
 * Funcao de hash
 * ========================================================= */

int hash_calcular_indice(int chave, int pg) {
    int mascara = potencia2(pg) - 1;
    return chave & mascara;
}

/* =========================================================
 * Insercao com split
 * ========================================================= */

/**
 * @brief Divide (split) um bucket cheio em dois.
 */
static int bucket_split(HashExtensivel *h, int indice_dir) {
    long offset_antigo = h->diretorio.offsets[indice_dir];
    Bucket antigo;
    if (bucket_ler(h->arquivo, offset_antigo, &antigo) != 0) return -2;

    int pl_novo = antigo.profundidade_local + 1;

    while (pl_novo > h->diretorio.profundidade_global) {
        if (diretorio_dobrar(&h->diretorio) != 0) return -2;
    }

    long offset_novo = bucket_novo(h->arquivo, pl_novo);
    if (offset_novo < 0) return -2;

    antigo.profundidade_local = pl_novo;

    Registro temp[BUCKET_CAPACIDADE];
    int n_temp = 0;
    for (int i = 0; i < BUCKET_CAPACIDADE; i++) {
        if (antigo.registros[i].chave != CHAVE_INVALIDA) {
            temp[n_temp++] = antigo.registros[i];
            antigo.registros[i].chave = CHAVE_INVALIDA;
        }
    }
    antigo.ocupacao = 0;

    Bucket novo;
    bucket_ler(h->arquivo, offset_novo, &novo);

    int pg         = h->diretorio.profundidade_global;
    int mascara_pl = potencia2(pl_novo) - 1;
    int bit_disc   = potencia2(pl_novo - 1);
    int sufixo_ant = indice_dir & (potencia2(pl_novo - 1) - 1);

    for (int i = 0; i < h->diretorio.tamanho; i++) {
        if (h->diretorio.offsets[i] == offset_antigo) {
            if ((i & mascara_pl) == (sufixo_ant | bit_disc)) {
                h->diretorio.offsets[i] = offset_novo;
            }
        }
    }

    for (int i = 0; i < n_temp; i++) {
        int idx = hash_calcular_indice(temp[i].chave, pg);
        long dest_offset = h->diretorio.offsets[idx];

        if (dest_offset == offset_antigo) {
            antigo.registros[antigo.ocupacao++] = temp[i];
        } else {
            novo.registros[novo.ocupacao++] = temp[i];
        }
    }

    bucket_escrever(h->arquivo, offset_antigo, &antigo);
    bucket_escrever(h->arquivo, offset_novo, &novo);

    return 0;
}

int hash_inserir(HashExtensivel *h, Registro reg) {
    if (!h) return -2;

    Registro tmp;
    if (hash_buscar(h, reg.chave, &tmp) == 0) return -1;

    while (1) {
        int idx    = hash_calcular_indice(reg.chave, h->diretorio.profundidade_global);
        long offset = h->diretorio.offsets[idx];

        Bucket b;
        if (bucket_ler(h->arquivo, offset, &b) != 0) return -2;

        if (b.ocupacao < BUCKET_CAPACIDADE) {
            for (int i = 0; i < BUCKET_CAPACIDADE; i++) {
                if (b.registros[i].chave == CHAVE_INVALIDA) {
                    b.registros[i] = reg;
                    b.ocupacao++;
                    bucket_escrever(h->arquivo, offset, &b);
                    return 0;
                }
            }
        }

        if (bucket_split(h, idx) != 0) return -2;
    }
}

/* =========================================================
 * Busca
 * ========================================================= */

int hash_buscar(HashExtensivel *h, int chave, Registro *saida) {
    if (!h) return -1;

    int idx    = hash_calcular_indice(chave, h->diretorio.profundidade_global);
    long offset = h->diretorio.offsets[idx];

    Bucket b;
    if (bucket_ler(h->arquivo, offset, &b) != 0) return -1;

    for (int i = 0; i < BUCKET_CAPACIDADE; i++) {
        if (b.registros[i].chave == chave) {
            if (saida) *saida = b.registros[i];
            return 0;
        }
    }
    return -1;
}

/* =========================================================
 * Remocao logica
 * ========================================================= */

int hash_remover(HashExtensivel *h, int chave) {
    if (!h) return -1;

    int idx    = hash_calcular_indice(chave, h->diretorio.profundidade_global);
    long offset = h->diretorio.offsets[idx];

    Bucket b;
    if (bucket_ler(h->arquivo, offset, &b) != 0) return -1;

    for (int i = 0; i < BUCKET_CAPACIDADE; i++) {
        if (b.registros[i].chave == chave) {
            b.registros[i].chave = CHAVE_INVALIDA;
            memset(b.registros[i].valor, 0, sizeof(b.registros[i].valor));
            b.ocupacao--;
            bucket_escrever(h->arquivo, offset, &b);
            return 0;
        }
    }
    return -1;
}

/* =========================================================
 * Debug / impressao
 * ========================================================= */

void hash_imprimir(HashExtensivel *h) {
    if (!h) return;

    printf("\n=== Hash Extensivel ===\n");
    printf("Profundidade Global: %d\n", h->diretorio.profundidade_global);
    printf("Tamanho do diretorio: %d\n\n", h->diretorio.tamanho);

    for (int i = 0; i < h->diretorio.tamanho; i++) {
        long offset = h->diretorio.offsets[i];
        Bucket b;
        if (bucket_ler(h->arquivo, offset, &b) != 0) continue;

        printf("Dir[%2d] -> offset=%ld | pl=%d | ocupacao=%d | registros: ",
               i, offset, b.profundidade_local, b.ocupacao);

        for (int j = 0; j < BUCKET_CAPACIDADE; j++) {
            if (b.registros[j].chave != CHAVE_INVALIDA) {
                printf("[%d:'%s'] ", b.registros[j].chave, b.registros[j].valor);
            }
        }
        printf("\n");
    }
    printf("======================\n\n");
}
