#ifndef UI_H
#define UI_H

#include <stdbool.h>

#include "raylib.h"

/*
 * Auxiliares de interface da versão gráfica (raylib): carregamento de fonte
 * com acentos, desenho de texto e botões. O restante do jogo não depende
 * daqui; só o menu gráfico usa este módulo.
 */

// Altura, em pixels, usada para rasterizar a fonte. Os textos podem ser
// desenhados em outros tamanhos a partir deste atlas.
#define UI_FONTE_TAMANHO_BASE 64

// Caminho procurado quando ui_carregar_fonte recebe NULL.
#define UI_CAMINHO_FONTE "assets/fontes/DejaVuSans.ttf"

typedef struct {
    Font fonte;
    int tamanho_base;
    bool carregada; // false quando o .ttf falhou e caiu na fonte padrão
} UIFonte;

// ---------------------------------------------------------------------------
// Fonte
// ---------------------------------------------------------------------------
// Codepoints usados pelo português: ASCII imprimível, Latin-1 Supplement,
// Latin Extended-A e a pontuação tipográfica do roteiro (— – “ ” …). O vetor
// é estático, pertence ao módulo e não deve ser liberado.
const int *ui_codepoints_portugues(int *quantidade);

// Procura o .ttf no caminho informado, na pasta do executável, no diretório
// atual e no diretório pai. Devolve NULL se não encontrar. Quando NULL é
// passado, procura UI_CAMINHO_FONTE nos mesmos lugares.
const char *ui_caminho_fonte(const char *caminho);

// Carrega o .ttf com todos os caracteres do português. Se o arquivo não for
// encontrado, devolve a fonte padrão da raylib e marca carregada = false.
// tamanho_base <= 0 usa UI_FONTE_TAMANHO_BASE.
UIFonte ui_carregar_fonte(const char *caminho, int tamanho_base);

// Libera a textura do atlas. A fonte padrão da raylib não é liberada.
void ui_descarregar_fonte(UIFonte *fonte);

// Confere se a fonte tem glifos próprios para "á ç ã é ô", sem cair no '?'.
bool ui_fonte_suporta_acentos(const UIFonte *fonte);

// ---------------------------------------------------------------------------
// Texto
// ---------------------------------------------------------------------------
// tamanho é a altura desejada em pixels (a fonte é escalada a partir do atlas).
Vector2 ui_medir_texto(const UIFonte *fonte, const char *texto, float tamanho);

void ui_desenhar_texto(const UIFonte *fonte, const char *texto, Vector2 posicao,
                       float tamanho, Color cor);

// Centraliza o texto horizontal e verticalmente dentro da área.
void ui_desenhar_texto_centralizado(const UIFonte *fonte, const char *texto,
                                    Rectangle area, float tamanho, Color cor);

// Desenha o texto quebrando linhas nos espaços para caber na largura da área.
// Respeita '\n'. Devolve a altura ocupada em pixels.
float ui_desenhar_texto_quebrado(const UIFonte *fonte, const char *texto,
                                 Rectangle area, float tamanho,
                                 float espacamento_linhas, Color cor);

// ---------------------------------------------------------------------------
// Botões
// ---------------------------------------------------------------------------
typedef struct {
    Rectangle area;
    const char *rotulo;
    bool habilitado;
} BotaoUI;

// Desenha o botão e devolve true quando ele foi acionado. O acionamento pode
// ser um clique do mouse dentro da área ou Enter/Espaço quando o botão está
// destacado (seleção por teclado).
bool ui_desenhar_botao(const UIFonte *fonte, const BotaoUI *botao,
                       bool destacado);

#endif
