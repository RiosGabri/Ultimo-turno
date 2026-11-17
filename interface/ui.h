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

typedef enum {
    UI_ICONE_CONCORDA,
    UI_ICONE_DISCORDA,
    UI_ICONE_NEUTRO,
    UI_ICONE_ALERTA
} UIIcone;

// Desenha o ícone centralizado em `centro`; `tamanho` é a largura/altura.
// Os símbolos são desenhados com formas e não dependem de glifos especiais.
void ui_desenhar_icone(UIIcone icone, Vector2 centro, float tamanho, Color cor);

// Desenha um painel e o texto quebrado em seu interior. O texto é recortado à
// área interna se exceder a altura disponível. Devolve a altura calculada.
float ui_desenhar_painel_texto(const UIFonte *fonte, Rectangle area,
                               const char *texto, float tamanho_texto,
                               Color fundo, Color borda, Color cor_texto);

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
    bool primario;    // ação principal: preenchimento vermelho
    bool pressionado; // estado interno; inicialize com false/zero
} BotaoUI;

// Entrada já lida da raylib. Separada para a lógica poder ser testada sem
// abrir janela.
typedef struct {
    Vector2 mouse;      // posição do mouse
    bool apertou;       // botão esquerdo pressionado neste frame
    bool pressionado;   // botão esquerdo continua pressionado
    bool soltou;        // botão esquerdo solto neste frame
    bool confirmar;     // Enter/Espaço (ativação por teclado)
} EntradaBotao;

typedef enum {
    BOTAO_NORMAL,
    BOTAO_HOVER,       // mouse em cima ou foco de teclado
    BOTAO_PRESSIONADO, // mouse apertado dentro do botão
    BOTAO_ACIONADO,    // clique concluído ou confirmação pelo teclado
    BOTAO_DESABILITADO
} EstadoBotao;

// Atualiza o estado interno do botão e devolve o estado visual/ação.
// O clique só conta se o mouse tiver sido apertado dentro do botão e solto
// ainda dentro. Enter/Espaço aciona quando `focado` é true.
EstadoBotao ui_atualizar_botao(BotaoUI *botao, bool focado,
                               const EntradaBotao *entrada);

// Desenha o botão e devolve true quando ele foi acionado.
bool ui_desenhar_botao(const UIFonte *fonte, BotaoUI *botao, bool focado);

// Navegação por teclado: devolve o próximo botão habilitado, dando a volta no
// fim. `passo` positivo avança (baixo/Tab) e negativo volta (cima/Shift+Tab).
// Retorna -1 se não houver botão habilitado.
int ui_navegar_botoes(const BotaoUI *botoes, int quantidade, int focado,
                      int passo);

#endif
