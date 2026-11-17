#include "telas.h"

#include <math.h>
#include <stdio.h>

#include "paleta.h"
#include "rlgl.h"

#include "tela_caderno.h"

#define LARGURA_TELA TELAS_LARGURA
#define ALTURA_TELA TELAS_ALTURA
#define TAMANHO_TITULO_TELA 56.0f
#define LARGURA_BOTAO 360
#define ALTURA_BOTAO 64
#define ESPACO_BOTOES 18

#define CARTA_LARGURA 410.0f
#define CARTA_ALTURA 500.0f
#define CARTA_ESPACO 56.0f
#define CARTA_TOPO 92.0f

typedef enum {
    TELA_MENU,
    TELA_HISTORIA,
    TELA_DETETIVE,
    TELA_MANHA,
    TELA_DEBATE,
    TELA_CADERNO
} Tela;

typedef enum {
    OPCAO_INICIAR,
    OPCAO_SAIR,
    TOTAL_OPCOES_MENU
} OpcaoMenu;

typedef enum {
    OPCAO_ABRIR_CADERNO,
    OPCAO_IR_DEBATE,
    TOTAL_OPCOES_MANHA
} OpcaoManha;

static const char *ROTULOS_MENU[TOTAL_OPCOES_MENU] = {
    [OPCAO_INICIAR] = "Iniciar investigação",
    [OPCAO_SAIR] = "Sair",
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

static void desenhar_fundo_historia(const Recursos *recursos) {
    if (recursos->fundo_historia.id != 0) {
        float escala = (float)ALTURA_TELA /
                       (float)recursos->fundo_historia.height;
        float largura = recursos->fundo_historia.width * escala;
        Rectangle origem = {0.0f, 0.0f,
                            (float)recursos->fundo_historia.width,
                            (float)recursos->fundo_historia.height};
        Rectangle destino = {LARGURA_TELA - largura, 0.0f, largura,
                             (float)ALTURA_TELA};
        DrawTexturePro(recursos->fundo_historia, origem, destino,
                       (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
    } else {
        recursos_desenhar_fundo(recursos);
    }

    DrawRectangleGradientH(0, 0, 900, ALTURA_TELA,
                           (Color){18, 18, 17, 238},
                           (Color){18, 18, 17, 0});
    DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA,
                  (Color){10, 10, 9, 20});
}

static void desenhar_historia(const UIFonte *fonte_titulo,
                              const UIFonte *fonte_corpo,
                              const UIFonte *fonte_corpo_bold,
                              const Recursos *recursos) {
    desenhar_fundo_historia(recursos);

    DrawRectangle(96, 68, 28, 2, COR_TIJOLO);
    ui_desenhar_texto(fonte_corpo_bold, "PRÓLOGO",
                      (Vector2){96.0f, 80.0f}, 13.0f, COR_TIJOLO);
    ui_desenhar_texto(fonte_titulo, "A FÁBRICA",
                      (Vector2){96.0f, 105.0f}, 68.0f, COR_OSSO);

    Rectangle area = {96.0f, 0.0f, 820.0f, 0.0f};
    float y = 210.0f;

    area.y = y;
    float altura = ui_desenhar_texto_quebrado(
        fonte_titulo,
        "Na década de 1930, a fábrica acorda antes do sol.",
        area, 30.0f, 6.0f, COR_OSSO);
    y += altura + 22.0f;

    area.y = y;
    altura = ui_desenhar_texto_quebrado(
        fonte_corpo,
        "O apito marca o turno, as máquinas cobrem as conversas e ninguém "
        "olha duas vezes para o chão.",
        area, 20.0f, 7.0f, COR_PAPEL);
    y += altura + 16.0f;

    area.y = y;
    altura = ui_desenhar_texto_quebrado(
        fonte_corpo,
        "Ao fim do expediente, Chaplin é encontrado morto na sala de "
        "máquinas. A polícia não quer escândalo. Os operários não querem "
        "falar.",
        area, 20.0f, 7.0f, COR_PAPEL);
    y += altura + 28.0f;

    DrawRectangle(96, (int)y + 2, 3, 26, COR_TIJOLO);
    Rectangle final_area = {112.0f, y, 800.0f, 0.0f};
    ui_desenhar_texto_quebrado(
        fonte_corpo_bold,
        "Você tem quatro dias para descobrir quem mentiu.",
        final_area, 20.0f, 5.0f, COR_OSSO);
}

static void desenhar_card_detetive(const UIFonte *fonte_titulo,
                                   const UIFonte *fonte_corpo,
                                   const UIFonte *fonte_corpo_bold,
                                   const Recursos *recursos, int id,
                                   Rectangle area, float escala,
                                   bool selecionado) {
    Texture2D retrato = (id == 0) ? recursos->retrato_marple
                                  : recursos->retrato_poirot;
    const char *nome = (id == 0) ? "MISS MARPLE" : "HERCULE POIROT";
    const char *perfil = (id == 0) ? "A OBSERVADORA" : "O METÓDICO";
    const char *citacao = (id == 0)
        ? "\"Pessoas mentem. Hábitos, não.\""
        : "\"O caos é apenas um padrão que ainda não foi organizado.\"";
    const char *descricao = (id == 0)
        ? "Leitura humana, memória e conversa."
        : "Ordem, contradição e dedução.";
    const char *arquivo = (id == 0) ? "DETETIVE 01" : "DETETIVE 02";

    Vector2 centro = {area.x + area.width * 0.5f,
                      area.y + area.height * 0.5f};

    rlPushMatrix();
    rlTranslatef(centro.x, centro.y, 0.0f);
    rlScalef(escala, escala, 1.0f);
    rlTranslatef(-centro.x, -centro.y, 0.0f);

    Rectangle sombra = {area.x + 8.0f, area.y + 10.0f, area.width,
                        area.height};
    DrawRectangleRec(sombra, COR_SOMBRA);
    DrawRectangleRec(area, COR_PAPEL);
    DrawRectangleLinesEx(area, selecionado ? 2.0f : 1.0f,
                         selecionado ? COR_TIJOLO : COR_POEIRA);

    Rectangle interno = {area.x + 6.0f, area.y + 6.0f, area.width - 12.0f,
                         area.height - 12.0f};
    DrawRectangleLinesEx(interno, 1.0f, (Color){138, 129, 117, 100});

    Rectangle aba = {area.x + 22.0f, area.y + 14.0f, 104.0f, 16.0f};
    DrawRectangleRec(aba, selecionado ? COR_TIJOLO : COR_TINTA);
    ui_desenhar_texto(fonte_corpo_bold, arquivo,
                      (Vector2){aba.x + 7.0f, aba.y + 3.0f}, 8.5f,
                      selecionado ? COR_OSSO : COR_POEIRA);

    Rectangle quadro = {area.x + 22.0f, area.y + 40.0f, area.width - 44.0f,
                        218.0f};
    DrawRectangleRec(quadro, COR_OSSO);
    DrawRectangleLinesEx(quadro, 1.0f, COR_POEIRA);

    Rectangle foto = {quadro.x + 8.0f, quadro.y + 8.0f, quadro.width - 16.0f,
                      quadro.height - 16.0f};
    if (retrato.id != 0) {
        float escala_foto =
            fmaxf(foto.width / (float)retrato.width,
                  foto.height / (float)retrato.height);
        float largura_origem = foto.width / escala_foto;
        float altura_origem = foto.height / escala_foto;
        Rectangle origem = {
            (retrato.width - largura_origem) * 0.5f,
            0.0f,
            largura_origem,
            altura_origem,
        };
        DrawTexturePro(retrato, origem, foto, (Vector2){0.0f, 0.0f}, 0.0f,
                       WHITE);
    }

    float marca = 10.0f;
    DrawLineEx((Vector2){quadro.x + 6, quadro.y + 6 + marca},
               (Vector2){quadro.x + 6, quadro.y + 6}, 1.0f, COR_POEIRA);
    DrawLineEx((Vector2){quadro.x + 6, quadro.y + 6},
               (Vector2){quadro.x + 6 + marca, quadro.y + 6}, 1.0f,
               COR_POEIRA);
    DrawLineEx((Vector2){quadro.x + quadro.width - 6 - marca,
                         quadro.y + 6},
               (Vector2){quadro.x + quadro.width - 6, quadro.y + 6}, 1.0f,
               COR_POEIRA);
    DrawLineEx((Vector2){quadro.x + quadro.width - 6, quadro.y + 6},
               (Vector2){quadro.x + quadro.width - 6, quadro.y + 6 + marca},
               1.0f, COR_POEIRA);

    Rectangle selo = {quadro.x + quadro.width - 134.0f,
                      quadro.y + quadro.height - 30.0f, 120.0f, 22.0f};
    DrawRectangleLinesEx(selo, 1.0f, COR_TIJOLO);
    ui_desenhar_texto(fonte_corpo_bold, "HOMOLOGADO",
                      (Vector2){selo.x + 7.0f, selo.y + 5.0f}, 9.0f,
                      COR_TIJOLO);

    ui_desenhar_texto(fonte_titulo, nome,
                      (Vector2){area.x + 22.0f, area.y + 276.0f}, 26.0f,
                      COR_TINTA);
    ui_desenhar_texto(fonte_corpo_bold, perfil,
                      (Vector2){area.x + 22.0f, area.y + 310.0f}, 10.5f,
                      COR_TIJOLO);

    DrawRectangle(area.x + 22, area.y + 330, area.width - 44, 1,
                  (Color){138, 129, 117, 120});

    Rectangle area_citacao = {area.x + 22.0f, area.y + 342.0f,
                              area.width - 44.0f, 0.0f};
    ui_desenhar_texto_quebrado(fonte_corpo, citacao, area_citacao, 12.5f,
                               2.0f, COR_TINTA);

    ui_desenhar_texto(fonte_corpo, descricao,
                      (Vector2){area.x + 22.0f, area.y + 408.0f}, 11.0f,
                      COR_TINTA);

    if (selecionado) {
        DrawRectangle(area.x + 22, area.y + 452, 4, 4, COR_TIJOLO);
        DrawRectangle(area.x + 30, area.y + 452, 4, 4, COR_TIJOLO);
        ui_desenhar_texto(fonte_corpo_bold, "POSIÇÃO SELECIONADA",
                          (Vector2){area.x + 42.0f, area.y + 448.0f}, 8.5f,
                          COR_TIJOLO);
    }

    rlPopMatrix();
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

static void desenhar_fundo_textura_com_vinheta(Texture2D textura,
                                               unsigned char intensidade) {
    if (textura.id != 0) {
        Rectangle tela = {0.0f, 0.0f, (float)LARGURA_TELA,
                          (float)ALTURA_TELA};
        recursos_desenhar_textura_cover(textura, tela);
    } else {
        ClearBackground(COR_CARVAO);
    }

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

static Tela tela = TELA_MENU;
static int selecionado = 0;
static bool fechar = false;
static bool jogo_ativo = false;
static const char *erro_inicio = NULL;
static Jogo jogo;

static UIFonte fonte_titulo;
static UIFonte fonte_corpo;
static UIFonte fonte_corpo_bold;
static Recursos recursos;

static BotaoUI botoes_menu[TOTAL_OPCOES_MENU];
static BotaoUI botao_avancar_historia;
static BotaoUI botao_iniciar_detetive;
static BotaoUI botoes_manha[TOTAL_OPCOES_MANHA];
static BotaoUI botao_debate_voltar;

static float escalas_detetives[2] = {1.06f, 0.98f};

static void desenhar_manha(void) {
    desenhar_fundo_textura_com_vinheta(recursos.fabrica, 150);

    char texto_dia[64];
    snprintf(texto_dia, sizeof(texto_dia), "DIA %d/%d — MANHÃ",
             jogo.dia.dia_atual, TOTAL_DIAS);
    ui_desenhar_texto(&fonte_corpo_bold, texto_dia,
                      (Vector2){96.0f, 64.0f}, 18.0f, COR_TIJOLO);
    ui_desenhar_texto(&fonte_titulo, "O TURNO COMEÇA",
                      (Vector2){96.0f, 100.0f}, 52.0f, COR_OSSO);

    Rectangle painel = {96.0f, 240.0f, 800.0f, 300.0f};
    DrawRectangleRec(painel, COR_TINTA);
    DrawRectangleLinesEx(painel, 1.0f, COR_POEIRA);

    ui_desenhar_texto(&fonte_corpo_bold, "PISTA DO DIA",
                      (Vector2){120.0f, 270.0f}, 13.0f, COR_POEIRA);

    const char *nome = nome_ultima_pista(&jogo);
    ui_desenhar_texto(&fonte_titulo,
                      (nome != NULL) ? nome : "A pista ainda não foi encontrada",
                      (Vector2){120.0f, 300.0f}, 32.0f, COR_OSSO);

    Rectangle descricao = {120.0f, 350.0f, 752.0f, 170.0f};
    ui_desenhar_texto_quebrado(&fonte_corpo, descricao_ultima_pista(&jogo),
                               descricao, 17.0f, 4.0f, COR_PAPEL);
}

static void desenhar_debate(void) {
    desenhar_fundo_textura_com_vinheta(recursos.fundo_detetives, 180);

    char titulo[64];
    snprintf(titulo, sizeof(titulo), "DEBATE — DIA %d/%d",
             jogo.dia.dia_atual, TOTAL_DIAS);
    desenhar_titulo_centralizado(&fonte_titulo, titulo, 64.0f, 52.0f,
                                 COR_OSSO);

    Suspeito *falante = suspeito_do_turno(&jogo);
    Rectangle painel = {160.0f, 230.0f, 960.0f, 260.0f};
    DrawRectangleRec(painel, COR_TINTA);
    DrawRectangleLinesEx(painel, 1.0f, COR_POEIRA);

    if (falante != NULL) {
        char vez[160];
        snprintf(vez, sizeof(vez), "Vez de %s (%s)", falante->nome,
                 falante->cargo);
        Rectangle area_vez = {180.0f, 270.0f, 920.0f, 52.0f};
        ui_desenhar_texto_centralizado(&fonte_titulo, vez, area_vez, 32.0f,
                                       COR_OSSO);
    } else {
        Rectangle area_fim = {180.0f, 270.0f, 920.0f, 52.0f};
        ui_desenhar_texto_centralizado(
            &fonte_titulo, "Todos os suspeitos vivos já falaram", area_fim,
            30.0f, COR_OSSO);
    }

    Rectangle dica = {200.0f, 350.0f, 880.0f, 100.0f};
    ui_desenhar_texto_quebrado(
        &fonte_corpo,
        "A mesa do debate está montada. Perguntas, reações e caderno entram "
        "na próxima etapa.",
        dica, 20.0f, 6.0f, COR_PAPEL);
}

void telas_iniciar(void) {
    tela = TELA_MENU;
    selecionado = 0;
    fechar = false;
    jogo_ativo = false;
    erro_inicio = NULL;
    jogo = (Jogo){0};
    escalas_detetives[0] = 1.06f;
    escalas_detetives[1] = 0.98f;

    fonte_titulo =
        ui_carregar_fonte("assets/fontes/PlayfairDisplay-Bold.ttf", 96);
    fonte_corpo =
        ui_carregar_fonte("assets/fontes/CourierPrime-Regular.ttf", 48);
    fonte_corpo_bold =
        ui_carregar_fonte("assets/fontes/CourierPrime-Bold.ttf", 48);

    if (!ui_fonte_suporta_acentos(&fonte_titulo) ||
        !ui_fonte_suporta_acentos(&fonte_corpo)) {
        TraceLog(LOG_WARNING,
                 "Interface: uma das fontes nao cobre todos os acentos.");
    }

    recursos_carregar(&recursos);

    inicializar_botoes(botoes_menu, ROTULOS_MENU, TOTAL_OPCOES_MENU,
                       96.0f, 470.0f);
    botoes_menu[OPCAO_INICIAR].area = (Rectangle){96.0f, 470.0f, 420.0f, 64.0f};
    botoes_menu[OPCAO_SAIR].area = (Rectangle){100.0f, 580.0f, 120.0f, 24.0f};
    botoes_menu[OPCAO_INICIAR].primario = true;

    botao_avancar_historia = (BotaoUI){
        .area = {1016.0f, 616.0f, 200.0f, 56.0f},
        .rotulo = "AVANÇAR >",
        .habilitado = true,
        .primario = true,
    };

    botao_iniciar_detetive = (BotaoUI){
        .area = {450.0f, 640.0f, 380.0f, 56.0f},
        .rotulo = "INICIAR INVESTIGAÇÃO >",
        .habilitado = true,
        .primario = true,
    };

    botoes_manha[OPCAO_ABRIR_CADERNO] = (BotaoUI){
        .area = {96.0f, 620.0f, 260.0f, 56.0f},
        .rotulo = "ABRIR CADERNO",
        .habilitado = true,
        .primario = false,
    };

    botoes_manha[OPCAO_IR_DEBATE] = (BotaoUI){
        .area = {900.0f, 620.0f, 300.0f, 56.0f},
        .rotulo = "IR PARA O DEBATE >",
        .habilitado = true,
        .primario = true,
    };

    botao_debate_voltar = (BotaoUI){
        .area = {64.0f, 620.0f, 240.0f, 56.0f},
        .rotulo = "VOLTAR AO MENU",
        .habilitado = true,
        .primario = false,
    };
}

void telas_executar(void) {
    BotaoUI *botoes_atuais = NULL;
    int quantidade_botoes = 0;

    switch (tela) {
        case TELA_MENU:
            botoes_atuais = botoes_menu;
            quantidade_botoes = TOTAL_OPCOES_MENU;
            break;
        case TELA_HISTORIA:
            botoes_atuais = &botao_avancar_historia;
            quantidade_botoes = 1;
            break;
        case TELA_DETETIVE:
            botoes_atuais = NULL;
            quantidade_botoes = 0;
            break;
        case TELA_MANHA:
            botoes_atuais = botoes_manha;
            quantidade_botoes = TOTAL_OPCOES_MANHA;
            break;
        case TELA_DEBATE:
            botoes_atuais = &botao_debate_voltar;
            quantidade_botoes = 1;
            break;
        case TELA_CADERNO:
            botoes_atuais = NULL;
            quantidade_botoes = 0;
            break;
    }

    if (quantidade_botoes > 0) {
        selecionado =
            atualizar_selecao(botoes_atuais, quantidade_botoes, selecionado);
    }

    BeginDrawing();
    ClearBackground(COR_CARVAO);

    int acionado = -1;
    switch (tela) {
        case TELA_MENU:
            desenhar_menu(&fonte_titulo, &fonte_corpo, &recursos);
            acionado = desenhar_menu_acoes(
                &fonte_corpo_bold, botoes_menu, selecionado);
            if (acionado == OPCAO_INICIAR) {
                tela = TELA_HISTORIA;
                selecionado = 0;
            } else if (acionado == OPCAO_SAIR) {
                fechar = true;
            }
            break;

        case TELA_HISTORIA:
            desenhar_historia(&fonte_titulo, &fonte_corpo,
                              &fonte_corpo_bold, &recursos);
            acionado = desenhar_botoes(
                &fonte_corpo_bold, &botao_avancar_historia, 1, selecionado);
            if (acionado == 0) {
                tela = TELA_DETETIVE;
                selecionado = 0;
                escalas_detetives[0] = 1.06f;
                escalas_detetives[1] = 0.98f;
            }
            break;

        case TELA_DETETIVE: {
            desenhar_fundo_textura_com_vinheta(recursos.fundo_detetives, 45);
            desenhar_titulo_centralizado(
                &fonte_titulo, "ESCOLHA SEU DETETIVE", 18.0f, 44.0f, COR_OSSO);

            float x0 = (LARGURA_TELA -
                        (CARTA_LARGURA * 2.0f + CARTA_ESPACO)) * 0.5f;
            Rectangle areas[2] = {
                {x0, CARTA_TOPO, CARTA_LARGURA, CARTA_ALTURA},
                {x0 + CARTA_LARGURA + CARTA_ESPACO, CARTA_TOPO,
                 CARTA_LARGURA, CARTA_ALTURA},
            };

            Vector2 mouse = GetMousePosition();
            bool clique = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

            if (IsKeyPressed(KEY_LEFT)) {
                selecionado = 0;
            }
            if (IsKeyPressed(KEY_RIGHT)) {
                selecionado = 1;
            }
            for (int i = 0; i < 2; i++) {
                if (clique && CheckCollisionPointRec(mouse, areas[i])) {
                    selecionado = i;
                }
            }

            float dt = GetFrameTime();
            for (int i = 0; i < 2; i++) {
                bool hover = CheckCollisionPointRec(mouse, areas[i]);
                float alvo = (i == selecionado)
                                 ? 1.06f
                                 : (hover ? 1.02f : 0.98f);
                float suavizacao = 1.0f - expf(-14.0f * dt);
                escalas_detetives[i] +=
                    (alvo - escalas_detetives[i]) * suavizacao;
            }

            int ordem[2] = {
                (selecionado == 0) ? 1 : 0,
                (selecionado == 0) ? 0 : 1,
            };
            for (int k = 0; k < 2; k++) {
                int i = ordem[k];
                desenhar_card_detetive(
                    &fonte_titulo, &fonte_corpo, &fonte_corpo_bold,
                    &recursos, i, areas[i], escalas_detetives[i],
                    i == selecionado);
            }

            bool clicou_iniciar = ui_desenhar_botao(
                &fonte_corpo_bold, &botao_iniciar_detetive, false);
            bool confirmar = clicou_iniciar || IsKeyPressed(KEY_ENTER) ||
                             IsKeyPressed(KEY_KP_ENTER) ||
                             IsKeyPressed(KEY_SPACE);

            if (confirmar) {
                int id_detetive = (selecionado == 0) ? ID_MISS_MARPLE
                                                     : ID_HERCULE_POIROT;
                if (iniciar_jogo(&jogo, id_detetive)) {
                    jogo_ativo = true;
                    erro_inicio = NULL;
                    tela = TELA_MANHA;
                    selecionado = 0;
                } else {
                    liberar_jogo(&jogo);
                    erro_inicio = "Não foi possível iniciar a partida.";
                    TraceLog(LOG_ERROR, "%s", erro_inicio);
                }
            }

            if (erro_inicio != NULL) {
                Rectangle area_erro = {0.0f, 700.0f, LARGURA_TELA, 14.0f};
                ui_desenhar_texto_centralizado(
                    &fonte_corpo, erro_inicio, area_erro, 12.0f, COR_TIJOLO);
            }
            break;
        }

        case TELA_MANHA:
            desenhar_manha();

            acionado = desenhar_botoes(
                &fonte_corpo_bold,
                botoes_manha,
                TOTAL_OPCOES_MANHA,
                selecionado
            );

            if (acionado == OPCAO_ABRIR_CADERNO) {
                tela = TELA_CADERNO;
                selecionado = 0;
            } else if (acionado == OPCAO_IR_DEBATE) {
                avancar_fase_jogo(&jogo);

                if (jogo.dia.fase_atual == FASE_DEBATE) {
                    tela = TELA_DEBATE;
                    selecionado = 0;
                }
            }
            break;
        case TELA_DEBATE:
            desenhar_debate();
            acionado = desenhar_botoes(
                &fonte_corpo_bold, &botao_debate_voltar, 1, selecionado);
            if (acionado == 0) {
                liberar_jogo(&jogo);
                jogo_ativo = false;
                tela = TELA_MENU;
                selecionado = 0;
            }
            break;
        case TELA_CADERNO:
            if (tela_caderno_atualizar_desenhar(&fonte_corpo, jogo.caderno)) {
                tela = TELA_MANHA;
                selecionado = OPCAO_ABRIR_CADERNO;
            }
            break;
    }

    EndDrawing();
}

void telas_liberar(void) {
    if (jogo_ativo) {
        liberar_jogo(&jogo);
        jogo_ativo = false;
    }

    recursos_descarregar(&recursos);
    ui_descarregar_fonte(&fonte_titulo);
    ui_descarregar_fonte(&fonte_corpo);
    ui_descarregar_fonte(&fonte_corpo_bold);
}

bool telas_deve_fechar(void) {
    return fechar;
}
