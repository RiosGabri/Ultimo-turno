#include "ui.h"

#include <stdio.h>

#include "jogo.h"
#include "paleta.h"
#include "recursos.h"

#define LARGURA_TELA 1280
#define ALTURA_TELA 720
#define TITULO_JOGO "Último Turno"
#define TAMANHO_TITULO_MENU 90.0f
#define TAMANHO_TITULO_TELA 56.0f
#define LARGURA_BOTAO 360
#define ALTURA_BOTAO 64
#define ESPACO_BOTOES 18

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
    [OPCAO_INICIAR] = "Iniciar investigação",
    [OPCAO_SAIR] = "Sair",
};

static const char *ROTULOS_DETETIVE[TOTAL_OPCOES_DETETIVE] = {
    [OPCAO_MISS_MARPLE] = "Miss Marple",
    [OPCAO_HERCULE_POIROT] = "Hercule Poirot",
    [OPCAO_VOLTAR] = "Voltar",
};

static Rectangle obter_retangulo_botao(int indice, float x, float primeiro_y) {
    float y = primeiro_y + indice * (ALTURA_BOTAO + ESPACO_BOTOES);
    return (Rectangle){x, y, LARGURA_BOTAO, ALTURA_BOTAO};
}

static void inicializar_botoes(BotaoUI *botoes, const char *const *rotulos,
                               int quantidade, float x, float primeiro_y) {
    for (int i = 0; i < quantidade; i++) {
        botoes[i] = (BotaoUI){
            .area = obter_retangulo_botao(i, x, primeiro_y),
            .rotulo = rotulos[i],
            .habilitado = true,
            .primario = false,
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

static void desenhar_titulo_centralizado(const UIFonte *fonte,
                                         const char *texto, float y,
                                         float tamanho, Color cor) {
    Rectangle area = {0.0f, y, LARGURA_TELA, tamanho + 14.0f};
    ui_desenhar_texto_centralizado(fonte, texto, area, tamanho, cor);
}

static void desenhar_fundo_com_vinheta(const Recursos *recursos,
                                       unsigned char intensidade) {
    recursos_desenhar_fundo(recursos);
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA,
                  (Color){18, 18, 17, intensidade});
}

static void desenhar_menu(const UIFonte *fonte_titulo,
                          const UIFonte *fonte_corpo, const Recursos *recursos) {
    recursos_desenhar_fundo(recursos);

    // Scrim lateral para o texto respirar sem virar um card.
    DrawRectangleGradientH(0, 0, 860, ALTURA_TELA,
                           (Color){18, 18, 17, 238},
                           (Color){18, 18, 17, 0});
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA,
                  (Color){10, 10, 9, 28});

    ui_desenhar_texto(fonte_corpo,
                      "UM CASO DE HERCULE POIROT E MISS MARPLE",
                      (Vector2){100.0f, 104.0f}, 14.0f, COR_POEIRA);
    ui_desenhar_texto(fonte_titulo, "O ÚLTIMO TURNO",
                      (Vector2){96.0f, 136.0f}, TAMANHO_TITULO_MENU,
                      COR_OSSO);
    ui_desenhar_texto(fonte_corpo, "Uma investigação na fábrica",
                      (Vector2){100.0f, 252.0f}, 24.0f, COR_PAPEL);

    DrawRectangle(100, 300, 72, 3, COR_TIJOLO);

    ui_desenhar_texto(fonte_corpo, "ENTER / CLIQUE PARA ESCOLHER",
                      (Vector2){100.0f, ALTURA_TELA - 42.0f}, 14.0f,
                      COR_POEIRA);
}

int main(void) {
    InitWindow(LARGURA_TELA, ALTURA_TELA, TITULO_JOGO);
    SetTargetFPS(60);

    UIFonte fonte_titulo =
        ui_carregar_fonte("assets/fontes/PlayfairDisplay-Bold.ttf", 96);
    UIFonte fonte_corpo =
        ui_carregar_fonte("assets/fontes/CourierPrime-Regular.ttf", 48);
    UIFonte fonte_corpo_bold =
        ui_carregar_fonte("assets/fontes/CourierPrime-Bold.ttf", 48);

    if (!ui_fonte_suporta_acentos(&fonte_titulo) ||
        !ui_fonte_suporta_acentos(&fonte_corpo)) {
        TraceLog(LOG_WARNING,
                 "Interface: uma das fontes nao cobre todos os acentos.");
    }

    Recursos recursos;
    recursos_carregar(&recursos);

    BotaoUI botoes_menu[TOTAL_OPCOES_MENU];
    BotaoUI botoes_detetive[TOTAL_OPCOES_DETETIVE];
    BotaoUI botao_voltar_partida;

    inicializar_botoes(botoes_menu, ROTULOS_MENU, TOTAL_OPCOES_MENU,
                       96.0f, 356.0f);
    botoes_menu[OPCAO_INICIAR].primario = true;

    inicializar_botoes(botoes_detetive, ROTULOS_DETETIVE,
                       TOTAL_OPCOES_DETETIVE,
                       (LARGURA_TELA - LARGURA_BOTAO) / 2.0f, 300.0f);

    botao_voltar_partida = (BotaoUI){
        .area = {(LARGURA_TELA - LARGURA_BOTAO) / 2.0f, 590.0f,
                 LARGURA_BOTAO, ALTURA_BOTAO},
        .rotulo = "Voltar ao menu",
        .habilitado = true,
        .primario = false,
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
        ClearBackground(COR_CARVAO);

        int acionado = -1;
        switch (tela) {
            case TELA_MENU:
                desenhar_menu(&fonte_titulo, &fonte_corpo, &recursos);
                acionado = desenhar_botoes(&fonte_corpo_bold, botoes_menu,
                                           TOTAL_OPCOES_MENU, selecionado);
                if (acionado == OPCAO_INICIAR) {
                    tela = TELA_DETETIVE;
                    selecionado = 0;
                } else if (acionado == OPCAO_SAIR) {
                    fechar = true;
                }
                break;

            case TELA_DETETIVE: {
                desenhar_fundo_com_vinheta(&recursos, 218);
                desenhar_titulo_centralizado(
                    &fonte_titulo, "ESCOLHA SEU DETETIVE", 74.0f,
                    TAMANHO_TITULO_TELA, COR_OSSO);

                Rectangle instrucao = {0.0f, 150.0f, LARGURA_TELA, 34.0f};
                ui_desenhar_texto_centralizado(
                    &fonte_corpo, "Quem conduz a investigação?", instrucao,
                    22.0f, COR_POEIRA);

                acionado = desenhar_botoes(
                    &fonte_corpo_bold, botoes_detetive,
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
                    Rectangle area_erro = {0.0f, 630.0f, LARGURA_TELA, 28.0f};
                    ui_desenhar_texto_centralizado(
                        &fonte_corpo, erro_inicio, area_erro, 18.0f,
                        COR_TIJOLO);
                }
                break;
            }

            case TELA_PARTIDA: {
                desenhar_fundo_com_vinheta(&recursos, 232);
                desenhar_titulo_centralizado(
                    &fonte_titulo, "INVESTIGAÇÃO INICIADA", 64.0f,
                    TAMANHO_TITULO_TELA, COR_OSSO);

                char texto_detetive[96];
                snprintf(texto_detetive, sizeof(texto_detetive),
                         "Detetive: %s", nome_detetive(jogo.id_detetive));
                Rectangle nome = {0.0f, 170.0f, LARGURA_TELA, 38.0f};
                ui_desenhar_texto_centralizado(
                    &fonte_corpo, texto_detetive, nome, 26.0f, COR_OSSO);

                char texto_fase[96];
                snprintf(texto_fase, sizeof(texto_fase), "Dia %d — %s",
                         jogo.dia.dia_atual,
                         nome_fase(jogo.dia.fase_atual));
                Rectangle fase = {0.0f, 218.0f, LARGURA_TELA, 34.0f};
                ui_desenhar_texto_centralizado(
                    &fonte_corpo, texto_fase, fase, 22.0f, COR_POEIRA);

                const char *pista = nome_ultima_pista(&jogo);
                Rectangle area_pista = {120.0f, 290.0f, 1040.0f, 140.0f};
                ui_desenhar_painel_texto(
                    &fonte_corpo, area_pista,
                    (pista != NULL) ? pista : "A investigação está pronta.",
                    20.0f, COR_TINTA, COR_POEIRA, COR_OSSO);

                acionado = desenhar_botoes(
                    &fonte_corpo_bold, &botao_voltar_partida, 1, selecionado);
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
    recursos_descarregar(&recursos);
    ui_descarregar_fonte(&fonte_titulo);
    ui_descarregar_fonte(&fonte_corpo);
    ui_descarregar_fonte(&fonte_corpo_bold);
    CloseWindow();
    return 0;
}
