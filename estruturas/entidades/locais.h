#ifndef LOCAIS_H
#define LOCAIS_H

/*
 * Tabela de locais da fábrica.
 *
 * Cada local tem um nome e a preposição correta a usar nas falas ("na" ou
 * "no"). A história se passa em 5 locais da fábrica; essa é a lista final.
 */
typedef struct {
    const char *nome;
    const char *preposicao; // "na" ou "no"
} Local;

/* Quantidade de locais cadastrados. */
int total_locais(void);

/* Local no índice dado, ou NULL se o índice estiver fora da tabela. */
const Local *local_por_indice(int indice);

/* Nome do local no índice dado, ou NULL se o índice estiver fora da tabela. */
const char *nome_local(int indice);

/* Preposição do local pelo nome. Retorna "em" para nome NULL/desconhecido. */
const char *preposicao_do_local(const char *nome);

#endif
