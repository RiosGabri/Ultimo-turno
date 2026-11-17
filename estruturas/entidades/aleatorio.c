#include "entidades/aleatorio.h"

#include <stddef.h>

/* Semente usada quando o chamador passa 0, já que o xorshift32 não sai de
 * zero. O valor é arbitrário (constante de Knuth). */
#define SEMENTE_PADRAO 0x9E3779B9u

void aleatorio_iniciar(Aleatorio *gerador, uint32_t semente) {
    if (gerador == NULL) {
        return;
    }

    gerador->estado = (semente != 0u) ? semente : SEMENTE_PADRAO;
}

uint32_t aleatorio_proximo(Aleatorio *gerador) {
    if (gerador == NULL) {
        return 0;
    }

    /* xorshift32: três deslocamentos e três XOR. Período de 2^32 - 1 para
     * qualquer estado diferente de zero. */
    uint32_t x = gerador->estado;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    gerador->estado = x;
    return x;
}

int aleatorio_intervalo(Aleatorio *gerador, int limite) {
    if (gerador == NULL || limite <= 0) {
        return 0;
    }

    /* O resto de divisão introduz um viés mínimo; para limites pequenos
     * (6 locais, por exemplo) ele é irrelevante para o jogo. */
    return (int)(aleatorio_proximo(gerador) % (uint32_t)limite);
}
