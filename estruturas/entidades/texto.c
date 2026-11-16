#include "texto.h"

#include <stdlib.h>
#include <string.h>

char *copiar_texto(const char *origem) {
    if (origem == NULL) {
        return NULL;
    }

    size_t tamanho = strlen(origem) + 1; // inclui o '\0'
    char *copia = malloc(tamanho);
    if (copia == NULL) {
        return NULL;
    }

    memcpy(copia, origem, tamanho);
    return copia;
}
