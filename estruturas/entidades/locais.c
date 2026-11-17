#include "entidades/locais.h"

#include <stddef.h>
#include <string.h>

/*
 * Lista provisória. Para trocar/adicionar locais, basta editar esta tabela:
 * o resto do jogo lê tudo por total_locais()/local_por_indice().
 */
static const Local LOCAIS[] = {
    { "Sala de máquinas",  "na" },
    { "Gerência",          "na" },
    { "Linha de produção", "na" },
    { "Vestiário",         "no" },
    { "Enfermaria",        "na" },
};

static const int TOTAL_LOCAIS = (int)(sizeof LOCAIS / sizeof LOCAIS[0]);

int total_locais(void) {
    return TOTAL_LOCAIS;
}

const Local *local_por_indice(int indice) {
    if (indice < 0 || indice >= TOTAL_LOCAIS) {
        return NULL;
    }
    return &LOCAIS[indice];
}

const char *nome_local(int indice) {
    const Local *local = local_por_indice(indice);
    return (local != NULL) ? local->nome : NULL;
}

const char *preposicao_do_local(const char *nome) {
    if (nome == NULL) {
        return "em";
    }

    for (int i = 0; i < TOTAL_LOCAIS; i++) {
        if (strcmp(LOCAIS[i].nome, nome) == 0) {
            return LOCAIS[i].preposicao;
        }
    }

    return "em";
}
