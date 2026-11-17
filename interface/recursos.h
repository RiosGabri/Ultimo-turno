#ifndef RECURSOS_H
#define RECURSOS_H

#include <stddef.h>

#include "raylib.h"

/*
 * Recursos visuais da versão gráfica. Centraliza caminhos, carregamento e
 * descarregamento para as telas não chamarem LoadTexture espalhado.
 */
typedef struct {
    Texture2D fabrica;
    Texture2D fundo_menu;
    Texture2D fundo_historia;
    Texture2D fundo_detetives;
    Texture2D titulo;
    Texture2D marple;
    Texture2D hercule;
} Recursos;

// Procura um caminho relativo na pasta do executável, no diretório atual e
// um/dois níveis acima. Devolve `destino` quando encontra, ou NULL.
const char *recursos_caminho(const char *relativo, char *destino,
                             size_t tamanho);

// Carrega todos os recursos. Texturas ausentes ficam com id 0 e são tratadas
// como placeholders pelas telas.
void recursos_carregar(Recursos *recursos);

void recursos_descarregar(Recursos *recursos);

// Desenha a fábrica em modo cover, cobrindo a tela sem esticar.
void recursos_desenhar_fundo(const Recursos *recursos);

// Desenha uma textura em modo cover dentro do retângulo de destino.
void recursos_desenhar_textura_cover(Texture2D textura, Rectangle destino);

#endif
