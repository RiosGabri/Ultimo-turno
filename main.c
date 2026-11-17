#include "ui.h"
#include "jogo.h"

#include <stdio.h>

#define LARGURA_TELA 960
#define ALTURA_TELA 540
#define TITULO_JOGO "Último Turno"
#define TAMANHO_TITULO 64
#define LARGURA_BOTAO 300
#define ALTURA_BOTAO 56
#define ESPACO_BOTOES 20

typedef enum {
    TELA_MENU,
    TELA_DETETIVE,
    TELA_PARTIDA
} Tela;

typedef enum {
    OPCAO_INICIAR,
    OPCAO_SAIR,
    TOTAL_OPCOES_MENU
} OpcaoMenu;

typedef enum {
    OPCAO_MISS_MARPLE,
    OPCAO_HERCULE_POIROT,
    OPCAO_VOLTAR,
    TOTAL_OPCOES_DETETIVE
} OpcaoDetetive;

static const char *ROTULOS_MENU[TOTAL_OPCOES_MENU] = {
    [OPCAO_INICIAR] = "Iniciar",
    [OPCAO_SAIR] = "Sair",
};

static const char *ROTULOS_DETETIVE[TOTAL_OPCOES_DETETIVE] = {
    [OPCAO_MISS_MARPLE] = "Miss Marple",
    [OPCAO_HERCULE_POIROT] = "Hercule Poirot",
    [OPCAO_VOLTAR] = "Voltar",
};

static Rectangle obter_retangulo_botao(int indice, float primeiro_y) {
    float x = (LARGURA_TELA - LARGURA_BOTAO) / 2.0f;
    float y = primeiro_y + indice * (ALTURA_BOTAO + ESPACO_BOTOES);
    return (Rectangle){x, y, LARGURA_BOTAO, ALTURA_BOTAO};
}

static void inicializar_botoes(BotaoUI *botoes, const char *const *rotulos,
                               int quantidade, float primeiro_y) {
    for (int i = 0; i < quantidade; i++) {
        botoes[i] = (BotaoUI){
            .area = obter_retangulo_botao(i, primeiro_y),
            .rotulo = rotulos[i],
            .habilitado = true,
        };
    }
}

static int atualizar_selecao(const BotaoUI *botoes, int quantidade,
                             int selecionado) {
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    if (IsKeyPressed(KEY_DOWN) || (IsKeyPressed(KEY_TAB) && !shift)) {
        selecionado = ui_navegar_botoes(botoes, quantidade, selecionado, 1);
    }
    if (IsKeyPressed(KEY_UP) || (IsKeyPressed(KEY_TAB) && shift)) {
        selecionado = ui_navegar_botoes(botoes, quantidade, selecionado, -1);
    }

    Vector2 movimento = GetMouseDelta();
    if (movimento.x != 0.0f || movimento.y != 0.0f ||
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        for (int i = 0; i < quantidade; i++) {
            if (botoes[i].habilitado &&
                CheckCollisionPointRec(mouse, botoes[i].area)) {
                selecionado = i;
            }
        }
    }

    return selecionado;
}

static int desenhar_botoes(const UIFonte *fonte, BotaoUI *botoes,
                           int quantidade, int selecionado) {
    int acionado = -1;
    for (int i = 0; i < quantidade; i++) {
        bool focado = i == selecionado;
        if (ui_desenhar_botao(fonte, &botoes[i], focado)) {
            acionado = i;
        }
    }
    return acionado;
}

static const char *nome_detetive(int id) {
    switch (id) {
        case ID_MISS_MARPLE: return "Miss Marple";
        case ID_HERCULE_POIROT: return "Hercule Poirot";
        default: return "Detetive desconhecida";
    }
}

static void desenhar_titulo(const UIFonte *fonte, const char *texto) {
    Rectangle area = {0.0f, 64.0f, LARGURA_TELA, 84.0f};
    ui_desenhar_texto_centralizado(fonte, texto, area, TAMANHO_TITULO,
                                   RAYWHITE);
}

int main(void) {
    InitWindow(LARGURA_TELA, ALTURA_TELA, TITULO_JOGO);
    SetTargetFPS(60);

    UIFonte fonte = ui_carregar_fonte(NULL, UI_FONTE_TAMANHO_BASE);
    if (!ui_fonte_suporta_acentos(&fonte)) {
        TraceLog(LOG_WARNING,
                 "A fonte carregada nao tem os acentos de \"á ç ã é ô\".");
    }

    BotaoUI botoes_menu[TOTAL_OPCOES_MENU];
    BotaoUI botoes_detetive[TOTAL_OPCOES_DETETIVE];
    BotaoUI botao_voltar_partida;
    inicializar_botoes(botoes_menu, ROTULOS_MENU, TOTAL_OPCOES_MENU, 210.0f);
    inicializar_botoes(botoes_detetive, ROTULOS_DETETIVE,
                       TOTAL_OPCOES_DETETIVE, 180.0f);
    botao_voltar_partida = (BotaoUI){
        .area = {(LARGURA_TELA - LARGURA_BOTAO) / 2.0f, 430.0f,
                 LARGURA_BOTAO, ALTURA_BOTAO},
        .rotulo = "Voltar ao menu",
        .habilitado = true,
    };

    Tela tela = TELA_MENU;
    int selecionado = 0;
    bool fechar = false;
    bool jogo_ativo = false;
    const char *erro_inicio = NULL;
    Jogo jogo = {0};

    while (!WindowShouldClose() && !fechar) {
        BotaoUI *botoes_atuais = NULL;
        int quantidade_botoes = 0;
        switch (tela) {
            case TELA_MENU:
                botoes_atuais = botoes_menu;
                quantidade_botoes = TOTAL_OPCOES_MENU;
                break;
            case TELA_DETETIVE:
                botoes_atuais = botoes_detetive;
                quantidade_botoes = TOTAL_OPCOES_DETETIVE;
                break;
            case TELA_PARTIDA:
                botoes_atuais = &botao_voltar_partida;
                quantidade_botoes = 1;
                break;
        }
        selecionado =
            atualizar_selecao(botoes_atuais, quantidade_botoes, selecionado);

        BeginDrawing();
        ClearBackground((Color){20, 20, 28, 255});

        int acionado = -1;
        switch (tela) {
            case TELA_MENU:
                desenhar_titulo(&fonte, TITULO_JOGO);
                acionado = desenhar_botoes(&fonte, botoes_menu,
                                           TOTAL_OPCOES_MENU, selecionado);
                if (acionado == OPCAO_INICIAR) {
                    tela = TELA_DETETIVE;
                    selecionado = 0;
                } else if (acionado == OPCAO_SAIR) {
                    fechar = true;
                }
                break;

            case TELA_DETETIVE: {
                desenhar_titulo(&fonte, "Escolha a detetive");
                Rectangle instrucao = {0.0f, 145.0f, LARGURA_TELA, 28.0f};
                ui_desenhar_texto_centralizado(
                    &fonte, "Quem conduz a investigação?", instrucao, 22.0f,
                    (Color){190, 190, 205, 255});
                acionado =
                    desenhar_botoes(&fonte, botoes_detetive,
                                    TOTAL_OPCOES_DETETIVE, selecionado);

                if (acionado == OPCAO_VOLTAR) {
                    tela = TELA_MENU;
                    selecionado = 0;
                } else if (acionado == OPCAO_MISS_MARPLE ||
                           acionado == OPCAO_HERCULE_POIROT) {
                    int id_detetive =
                        (acionado == OPCAO_MISS_MARPLE)
                            ? ID_MISS_MARPLE
                            : ID_HERCULE_POIROT;
                    if (iniciar_jogo(&jogo, id_detetive)) {
                        jogo_ativo = true;
                        erro_inicio = NULL;
                        tela = TELA_PARTIDA;
                        selecionado = 0;
                    } else {
                        liberar_jogo(&jogo);
                        erro_inicio = "Não foi possível iniciar a partida.";
                        TraceLog(LOG_ERROR, "%s", erro_inicio);
                    }
                }

                if (erro_inicio != NULL) {
                    Rectangle area_erro = {0.0f, 470.0f, LARGURA_TELA, 28.0f};
                    ui_desenhar_texto_centralizado(
                        &fonte, erro_inicio, area_erro, 18.0f,
                        (Color){235, 105, 105, 255});
                }
                break;
            }

            case TELA_PARTIDA: {
                desenhar_titulo(&fonte, "Investigação iniciada");
                Rectangle nome = {80.0f, 180.0f, 800.0f, 38.0f};
                char texto_detetive[96];
                snprintf(texto_detetive, sizeof(texto_detetive),
                         "Detetive: %s", nome_detetive(jogo.id_detetive));
                ui_desenhar_texto_centralizado(&fonte, texto_detetive, nome,
                                               26.0f, RAYWHITE);

                char texto_fase[96];
                snprintf(texto_fase, sizeof(texto_fase), "Dia %d — %s",
                         jogo.dia.dia_atual,
                         nome_fase(jogo.dia.fase_atual));
                Rectangle fase = {80.0f, 230.0f, 800.0f, 34.0f};
                ui_desenhar_texto_centralizado(
                    &fonte, texto_fase, fase, 22.0f,
                    (Color){190, 190, 205, 255});

                const char *pista = nome_ultima_pista(&jogo);
                Rectangle area_pista = {150.0f, 290.0f, 660.0f, 90.0f};
                ui_desenhar_painel_texto(
                    &fonte, area_pista,
                    (pista != NULL) ? pista : "A investigação está pronta.",
                    20.0f, (Color){30, 30, 42, 255},
                    (Color){90, 90, 115, 255}, RAYWHITE);

                acionado = desenhar_botoes(&fonte, &botao_voltar_partida, 1,
                                           selecionado);
                if (acionado == 0) {
                    liberar_jogo(&jogo);
                    jogo_ativo = false;
                    tela = TELA_MENU;
                    selecionado = 0;
                }
                break;
            }
        }

        EndDrawing();
    }

    if (jogo_ativo) {
        liberar_jogo(&jogo);
    }
    ui_descarregar_fonte(&fonte);
    CloseWindow();
    return 0;
}
