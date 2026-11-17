#include "ui.h"

#include <stdio.h>

#include "jogo.h"
#include "paleta.h"
#include "recursos.h"

#define LARGURA_TELA 1280
#define ALTURA_TELA 720
#define TITULO_JOGO "Último Turno"
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

static EntradaBotao ler_entrada_botao(void) {
    return (EntradaBotao){
        .mouse = GetMousePosition(),
        .apertou = IsMouseButtonPressed(MOUSE_BUTTON_LEFT),
        .pressionado = IsMouseButtonDown(MOUSE_BUTTON_LEFT),
        .soltou = IsMouseButtonReleased(MOUSE_BUTTON_LEFT),
        .confirmar = IsKeyPressed(KEY_ENTER) ||
                     IsKeyPressed(KEY_KP_ENTER) ||
                     IsKeyPressed(KEY_SPACE),
    };
}

static int desenhar_menu_acoes(const UIFonte *fonte, BotaoUI *botoes,
                               int selecionado) {
    EntradaBotao entrada = ler_entrada_botao();
    int acionado = -1;

    BotaoUI *iniciar = &botoes[OPCAO_INICIAR];
    EstadoBotao estado_iniciar =
        ui_atualizar_botao(iniciar, selecionado == OPCAO_INICIAR, &entrada);

    Color fundo = COR_TIJOLO;
    Color borda = COR_TIJOLO;

    if (estado_iniciar == BOTAO_HOVER) {
        fundo = COR_OXIDO;
        borda = COR_OSSO;
    } else if (estado_iniciar == BOTAO_ACIONADO ||
               estado_iniciar == BOTAO_PRESSIONADO) {
        fundo = COR_SANGUE;
        borda = COR_OSSO;
    }

    if (selecionado == OPCAO_INICIAR) {
        borda = COR_OSSO;
    }

    DrawRectangleRec(iniciar->area, fundo);
    DrawRectangleLinesEx(iniciar->area, 1.0f, borda);

    Rectangle pip = {
        iniciar->area.x + 28.0f,
        iniciar->area.y + iniciar->area.height * 0.5f - 5.0f,
        10.0f,
        10.0f,
    };
    DrawRectangleRec(pip, COR_OSSO);

    const char *texto_iniciar = "INICIAR INVESTIGAÇÃO";
    Vector2 tamanho_iniciar =
        MeasureTextEx(fonte->fonte, texto_iniciar, 20.0f, 2.0f);
    DrawTextEx(fonte->fonte, texto_iniciar,
               (Vector2){iniciar->area.x + 54.0f,
                         iniciar->area.y +
                             (iniciar->area.height - tamanho_iniciar.y) * 0.5f},
               20.0f, 2.0f, COR_OSSO);

    if (estado_iniciar == BOTAO_ACIONADO) {
        acionado = OPCAO_INICIAR;
    }

    BotaoUI *sair = &botoes[OPCAO_SAIR];
    EstadoBotao estado_sair =
        ui_atualizar_botao(sair, selecionado == OPCAO_SAIR, &entrada);

    Color cor_sair = COR_POEIRA;
    if (selecionado == OPCAO_SAIR || estado_sair == BOTAO_HOVER) {
        cor_sair = COR_TIJOLO;
    } else if (estado_sair == BOTAO_PRESSIONADO ||
               estado_sair == BOTAO_ACIONADO) {
        cor_sair = COR_OSSO;
    }

    ui_desenhar_texto(fonte, "SAIR", (Vector2){sair->area.x, sair->area.y},
                      14.0f, cor_sair);

    if (estado_sair == BOTAO_ACIONADO) {
        acionado = OPCAO_SAIR;
    }

    return acionado;
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
    if (recursos->fundo_menu.id != 0) {
        Rectangle tela = {0.0f, 0.0f, (float)LARGURA_TELA,
                          (float)ALTURA_TELA};
        recursos_desenhar_textura_cover(recursos->fundo_menu, tela);
    } else {
        recursos_desenhar_fundo(recursos);
    }

    // Scrim lateral para o texto respirar sem virar um card.
    DrawRectangleGradientH(0, 0, 780, ALTURA_TELA,
                           (Color){18, 18, 17, 218},
                           (Color){18, 18, 17, 0});
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA,
                  (Color){10, 10, 9, 24});

    DrawRectangle(96, 78, 28, 2, COR_TIJOLO);

    if (recursos->titulo.id != 0) {
        Rectangle destino = {96.0f, 96.0f, 500.0f, 282.0f};
        Rectangle origem = {0.0f, 0.0f, (float)recursos->titulo.width,
                            (float)recursos->titulo.height};
        DrawTexturePro(recursos->titulo, origem, destino,
                       (Vector2){0.0f, 0.0f}, 0.0f, COR_OSSO);
    } else {
        ui_desenhar_texto(fonte_titulo, "O ÚLTIMO TURNO",
                          (Vector2){96.0f, 120.0f}, 72.0f, COR_OSSO);
    }

    ui_desenhar_texto(
        fonte_corpo, "Uma investigação na fábrica — Novembro de 1936",
        (Vector2){100.0f, 400.0f}, 17.0f, COR_PAPEL);
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
                       96.0f, 470.0f);
    botoes_menu[OPCAO_INICIAR].area = (Rectangle){96.0f, 470.0f, 420.0f, 64.0f};
    botoes_menu[OPCAO_SAIR].area = (Rectangle){100.0f, 580.0f, 120.0f, 24.0f};
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
                acionado = desenhar_menu_acoes(
                    &fonte_corpo_bold, botoes_menu, selecionado);
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
