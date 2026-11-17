#ifndef PALETA_H
#define PALETA_H

#include "raylib.h"

/*
 * Paleta visual do jogo: noir industrial com papel envelhecido e um único
 * acento vermelho. Os nomes são semânticos para a UI não depender de
 * valores soltos em cada tela.
 */

#define COR_CARVAO   ((Color){ 43,  43,  40, 255}) // fundo principal
#define COR_TINTA    ((Color){ 27,  27,  24, 255}) // painéis, overlays, sombras
#define COR_FUMACA   ((Color){ 58,  57,  52, 255}) // superfícies elevadas
#define COR_PAPEL    ((Color){216, 207, 188, 255}) // papel/fundo claro
#define COR_OSSO     ((Color){240, 234, 223, 255}) // texto de alta ênfase
#define COR_POEIRA   ((Color){138, 129, 117, 255}) // texto secundário/bordas
#define COR_TIJOLO   ((Color){194,  59,  34, 255}) // acento único
#define COR_OXIDO    ((Color){168,  50,  29, 255}) // acento pressionado
#define COR_SANGUE   ((Color){127,  36,  23, 255}) // vermelho profundo
#define COR_RUBOR    ((Color){194,  59,  34,  31}) // wash de alerta/contradição
#define COR_SOMBRA   ((Color){ 15,  15,  14, 140}) // sombra quente, sem preto puro

// Aliases semânticos usados pela interface.
#define COR_FUNDO              COR_CARVAO
#define COR_PAINEL             COR_TINTA
#define COR_TEXTO              COR_OSSO
#define COR_TEXTO_SECUNDARIO   COR_POEIRA
#define COR_ACCENT             COR_TIJOLO
#define COR_ACCENT_PRESSIONADO COR_OXIDO

#endif
