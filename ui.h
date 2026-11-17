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

// ---------------------------------------------------------------------------
// Quebra de linha
// ---------------------------------------------------------------------------
// Uma linha do layout aponta para dentro do texto original; nada é copiado.
typedef struct {
    const char *inicio; // primeiro byte da linha
    int comprimento;    // bytes da linha, sem o '\n' final
    float largura;      // largura medida, em pixels
} LinhaTexto;

// Resultado de ui_quebrar_texto. `linhas` é alocado pela função e deve ser
// liberado com ui_liberar_layout_texto.
typedef struct {
    LinhaTexto *linhas;
    int quantidade;
    float tamanho;            // tamanho de texto usado no cálculo
    float espacamento_linhas; // espaço extra entre linhas
    float altura;             // altura total ocupada
} LayoutTexto;

// Quebra o texto para caber em `largura_maxima` pixels. Respeita '\n', ignora
// espaços no começo das linhas e parte palavras maiores que a largura (sem
// hifenizar), para o texto nunca estourar a área.
LayoutTexto ui_quebrar_texto(const UIFonte *fonte, const char *texto,
                             float largura_maxima, float tamanho,
                             float espacamento_linhas);

void ui_liberar_layout_texto(LayoutTexto *layout);

// Desenha um layout já calculado. Devolve a altura desenhada. Útil para
// recalcular a quebra só quando o texto ou a largura mudarem.
float ui_desenhar_layout_texto(const UIFonte *fonte, const LayoutTexto *layout,
                               Vector2 posicao, Color cor);

// Atalho que calcula, desenha e libera o layout. Só a largura da área limita
// a quebra; a altura não corta o texto. Devolve a altura ocupada.
float ui_desenhar_texto_quebrado(const UIFonte *fonte, const char *texto,
                                 Rectangle area, float tamanho,
                                 float espacamento_linhas, Color cor);

// Altura que o texto ocuparia, sem desenhar.
float ui_medir_altura_texto_quebrado(const UIFonte *fonte, const char *texto,
                                     float largura_maxima, float tamanho,
                                     float espacamento_linhas);

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
