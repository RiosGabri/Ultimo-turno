#ifndef ALEATORIO_H
#define ALEATORIO_H

#include <stdint.h>

/*
 * Gerador pseudoaleatório simples e determinístico (xorshift32).
 *
 * Serve para sorteios que precisam ser reproduzíveis a partir de uma semente:
 * a mesma semente gera sempre a mesma sequência de números. É independente de
 * rand()/srand(), então a ordem das chamadas aqui não interfere nos outros
 * sorteios do jogo e vice-versa.
 *
 * Uso:
 *   Aleatorio gerador;
 *   aleatorio_iniciar(&gerador, semente);
 *   int local = aleatorio_intervalo(&gerador, TOTAL_LOCAIS); // 0 .. TOTAL_LOCAIS-1
 */
typedef struct {
    uint32_t estado;
} Aleatorio;

/* Reinicia o gerador com a semente dada. Semente 0 é trocada por um valor
 * padrão, porque o xorshift32 ficaria preso em zero. */
void aleatorio_iniciar(Aleatorio *gerador, uint32_t semente);

/* Próximo número da sequência. Retorna 0 se o gerador for NULL. */
uint32_t aleatorio_proximo(Aleatorio *gerador);

/* Inteiro em [0, limite - 1]. Retorna 0 se o gerador for NULL ou limite <= 0. */
int aleatorio_intervalo(Aleatorio *gerador, int limite);

#endif
