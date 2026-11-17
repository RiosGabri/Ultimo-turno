#include "ui.h"

#include <stdlib.h>

#define LARGURA_TELA 960
#define ALTURA_TELA 540
#define TITULO_JOGO "Último Turno"
#define TAMANHO_TITULO 64

#define TEXTO_RODAPE "Menu gráfico em construção — jogue no terminal com: make run"
#define TAMANHO_RODAPE 18

// Amostra visível para conferir os acentos da fonte carregada.
#define TEXTO_ACENTOS "Acentos: á ç ã é ô — ação, café, pão, avô"
#define TAMANHO_ACENTOS 20

#define LARGURA_BOTAO 300
#define ALTURA_BOTAO 56
#define ESPACO_BOTOES 20

typedef enum {
    OPCAO_INICIAR,
    OPCAO_CONFIGURACOES,
    OPCAO_TRILHA,
    OPCAO_SAIR,
    TOTAL_BOTOES
} OpcaoMenu;

static const char *ROTULOS[TOTAL_BOTOES] = {
    [OPCAO_INICIAR] = "Iniciar",
    [OPCAO_CONFIGURACOES] = "Configurações",
    [OPCAO_TRILHA] = "Trilha",
    [OPCAO_SAIR] = "Sair",
};

static Rectangle obter_retangulo_botao(int indice) {
    float x = (LARGURA_TELA - LARGURA_BOTAO) / 2.0f;
    float y = 170.0f + indice * (ALTURA_BOTAO + ESPACO_BOTOES);
    return (Rectangle){x, y, LARGURA_BOTAO, ALTURA_BOTAO};
}

static int atualizar_selecao(const BotaoUI *botoes, int selecionado) {
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    if (IsKeyPressed(KEY_DOWN) || (IsKeyPressed(KEY_TAB) && !shift)) {
        selecionado =
            ui_navegar_botoes(botoes, TOTAL_BOTOES, selecionado, 1);
    }
    if (IsKeyPressed(KEY_UP) || (IsKeyPressed(KEY_TAB) && shift)) {
        selecionado =
            ui_navegar_botoes(botoes, TOTAL_BOTOES, selecionado, -1);
    }

    Vector2 movimento = GetMouseDelta();
    if (movimento.x != 0.0f || movimento.y != 0.0f ||
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        for (int i = 0; i < TOTAL_BOTOES; i++) {
            if (botoes[i].habilitado &&
                CheckCollisionPointRec(mouse, botoes[i].area)) {
                selecionado = i;
            }
        }
    }

    return selecionado;
}

int main(void) {
    InitWindow(LARGURA_TELA, ALTURA_TELA, TITULO_JOGO);
    SetTargetFPS(60);

    UIFonte fonte = ui_carregar_fonte(NULL, UI_FONTE_TAMANHO_BASE);
    if (!ui_fonte_suporta_acentos(&fonte)) {
        TraceLog(LOG_WARNING,
                 "A fonte carregada nao tem os acentos de \"á ç ã é ô\".");
    }

    BotaoUI botoes[TOTAL_BOTOES];
    for (int i = 0; i < TOTAL_BOTOES; i++) {
        botoes[i] = (BotaoUI){
            .area = obter_retangulo_botao(i),
            .rotulo = ROTULOS[i],
            .habilitado = true,
        };
    }

    int selecionado = 0;
    bool fechar = false;

    while (!WindowShouldClose() && !fechar) {
        selecionado = atualizar_selecao(botoes, selecionado);

        BeginDrawing();
        ClearBackground((Color){20, 20, 28, 255});

        Rectangle area_titulo = {0.0f, 70.0f, LARGURA_TELA, 80.0f};
        ui_desenhar_texto_centralizado(&fonte, TITULO_JOGO, area_titulo,
                                       TAMANHO_TITULO, RAYWHITE);

        for (int i = 0; i < TOTAL_BOTOES; i++) {
            bool acionado =
                ui_desenhar_botao(&fonte, &botoes[i], i == selecionado);
            if (acionado && i == OPCAO_SAIR) {
                fechar = true;
            }
        }

        Rectangle area_acentos = {0.0f, ALTURA_TELA - 66.0f, LARGURA_TELA,
                                  TAMANHO_ACENTOS};
        ui_desenhar_texto_centralizado(&fonte, TEXTO_ACENTOS, area_acentos,
                                       TAMANHO_ACENTOS,
                                       (Color){150, 150, 175, 255});

        Rectangle area_rodape = {0.0f, ALTURA_TELA - 32.0f, LARGURA_TELA,
                                 TAMANHO_RODAPE};
        ui_desenhar_texto_centralizado(&fonte, TEXTO_RODAPE, area_rodape,
                                       TAMANHO_RODAPE, GRAY);

        EndDrawing();
    }

    ui_descarregar_fonte(&fonte);
    CloseWindow();
    return 0;
}
