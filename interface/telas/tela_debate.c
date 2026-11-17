#include "interface/telas/tela_debate.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "interface/paleta.h"
#include "interface/recursos.h"
#include "interface/ui.h"
#include "raylib.h"

/*
 * Tela do debate no padrão visual da main: 1280×720, paleta noir industrial,
 * Playfair Display nos títulos e Courier Prime no corpo.
 *
 * O cenário mantém o rascunho do time: uma mesa no centro, os suspeitos ao
 * redor e o detetive em primeira pessoa (não desenhado). O triângulo vermelho
 * marca o alvo da vez. Quando um sprite não está disponível, a tela cai em
 * uma silhueta desenhada com a inicial do suspeito.
 */

#define LARGURA_TELA 1280.0f
#define ALTURA_TELA 720.0f
#define ALTURA_CABECALHO 96.0f
#define ALTURA_BARRA_ACOES 84.0f
#define TOPO_BARRA_ACOES (ALTURA_TELA - ALTURA_BARRA_ACOES)
#define MARGEM 96.0f
#define LARGURA_CONTEUDO (LARGURA_TELA - MARGEM * 2.0f)

static const char *ROTULOS_PERIODO[TOTAL_PERIODOS] = {
    [MANHA] = "MANHÃ",
    [TARDE] = "TARDE",
    [NOITE] = "NOITE",
};

static const char *TEXTO_SELECIONE =
    "Selecione quem você quer interrogar";
static const char *TEXTO_SEM_TURNOS =
    "Todos os suspeitos vivos já falaram hoje.";
static const char *TEXTO_NOVA_CONTRADICAO =
    "Nova contradição registrada no caderno!";
static const char *TEXTO_RODA_ENCERRADA =
    "A roda terminou. Encerre o debate para seguir.";
static const char *TEXTO_DEPOIMENTO_VAZIO = "Depoimento sem texto.";
static const char *TEXTO_SEM_REACOES = "Ninguém reagiu.";

// ---------------------------------------------------------------------------
// Consultas do estado
// ---------------------------------------------------------------------------
static bool assento_valido(int id) {
    return id >= 0 && id < TOTAL_SUSPEITOS;
}

static Texture2D textura_suspeito(const TelaDebate *tela, int id) {
    if (tela == NULL || tela->recursos == NULL || !assento_valido(id)) {
        return (Texture2D){0};
    }
    return tela->recursos->retratos_suspeitos[id];
}

static const char *nome_de(const Suspeito *suspeito) {
    if (suspeito == NULL) {
        return "desconhecido";
    }
    if (suspeito->nome != NULL && suspeito->nome[0] != '\0') {
        return suspeito->nome;
    }
    if (suspeito->cargo != NULL && suspeito->cargo[0] != '\0') {
        return suspeito->cargo;
    }
    return "desconhecido";
}

static const char *cargo_de(const Suspeito *suspeito) {
    if (suspeito == NULL) {
        return "";
    }
    if (suspeito->cargo != NULL && suspeito->cargo[0] != '\0') {
        return suspeito->cargo;
    }
    return "";
}

// Evita repetir a mesma palavra quando nome e cargo coincidem (a main passou
// a usar apenas o cargo como nome dos suspeitos).
static const char *cargo_para_exibir(const Suspeito *suspeito) {
    const char *cargo = cargo_de(suspeito);
    if (cargo[0] == '\0' || strcmp(cargo, nome_de(suspeito)) == 0) {
        return "";
    }
    return cargo;
}

static const Suspeito *suspeito_vivo(const Jogo *jogo, int id) {
    const Suspeito *suspeito = buscar_suspeito(jogo, id);
    if (suspeito == NULL || suspeito->situacao != VIVO) {
        return NULL;
    }
    return suspeito;
}

static int primeiro_vivo(const Jogo *jogo) {
    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        if (suspeito_vivo(jogo, id) != NULL) {
            return id;
        }
    }
    return -1;
}

static int alvo_indicado(const TelaDebate *tela) {
    if (tela == NULL ||
        tela->etapa == DEBATE_MOSTRANDO_DEPOIMENTO ||
        debate_encerrado(tela->jogo)) {
        return -1;
    }

    if (suspeito_vivo(tela->jogo, tela->id_alvo) != NULL) {
        return tela->id_alvo;
    }

    return primeiro_vivo(tela->jogo);
}

static const char *nome_periodo(enum Periodo periodo) {
    switch (periodo) {
        case MANHA: return "manhã";
        case TARDE: return "tarde";
        case NOITE: return "noite";
        default:    return "período";
    }
}

static const char *nome_por_id(const Jogo *jogo, int id) {
    return nome_de(buscar_suspeito(jogo, id));
}

static UIIcone icone_da_reacao(enum TipoReacao tipo) {
    switch (tipo) {
        case CONCORDA: return UI_ICONE_CONCORDA;
        case DISCORDA: return UI_ICONE_DISCORDA;
        default:       return UI_ICONE_NEUTRO;
    }
}

static const char *rotulo_da_reacao(enum TipoReacao tipo) {
    switch (tipo) {
        case CONCORDA: return "CONCORDA";
        case DISCORDA: return "DISCORDA";
        default:       return "NEUTRO";
    }
}

static Color cor_da_reacao(enum TipoReacao tipo) {
    switch (tipo) {
        case CONCORDA: return COR_OSSO;
        case DISCORDA: return COR_TIJOLO;
        default:       return COR_POEIRA;
    }
}

// ---------------------------------------------------------------------------
// Layout do cenário
// ---------------------------------------------------------------------------
// "base" é o ponto em que os pés tocam o chão. A mesa é desenhada depois para
// esconder as pernas de quem está do outro lado dela.
typedef struct {
    Vector2 base;
    float altura;
} AssentoSuspeito;

static const AssentoSuspeito ASSENTOS[TOTAL_SUSPEITOS] = {
    [ID_GERENTE]    = {{170.0f, 578.0f}, 320.0f}, // frente esquerda
    [ID_CAPATAZ]    = {{340.0f, 540.0f}, 280.0f},
    [ID_ENGENHEIRO] = {{520.0f, 505.0f}, 250.0f}, // fundo esquerda
    [ID_OPERARIO]   = {{760.0f, 505.0f}, 250.0f}, // fundo direita
    [ID_ENFERMEIRA] = {{940.0f, 540.0f}, 280.0f},
    [ID_VIGIA]      = {{1110.0f, 578.0f}, 320.0f}, // frente direita
};

static Vector2 posicao_suspeito(int id) {
    if (!assento_valido(id)) {
        return (Vector2){LARGURA_TELA / 2.0f, 520.0f};
    }
    return ASSENTOS[id].base;
}

static float altura_suspeito(int id) {
    if (!assento_valido(id)) {
        return 250.0f;
    }
    return ASSENTOS[id].altura;
}

static float largura_suspeito(const TelaDebate *tela, int id) {
    float altura = altura_suspeito(id);
    float aspecto = 0.80f;

    Texture2D textura = textura_suspeito(tela, id);
    if (textura.id != 0 && textura.height > 0) {
        aspecto = (float)textura.width / (float)textura.height;
    }

    return altura * aspecto;
}

// A área clicável inclui a placa de nome acima da cabeça. Assim o jogador não
// precisa acertar exatamente o sprite.
static Rectangle area_suspeito(const TelaDebate *tela, int id) {
    Vector2 base = posicao_suspeito(id);
    float altura = altura_suspeito(id);
    float largura = fmaxf(largura_suspeito(tela, id), 110.0f);

    return (Rectangle){
        base.x - largura / 2.0f - 10.0f,
        base.y - altura - 84.0f,
        largura + 20.0f,
        altura + 92.0f,
    };
}

static float topo_suspeito(int id) {
    return posicao_suspeito(id).y - altura_suspeito(id);
}

// ---------------------------------------------------------------------------
// Pequenos auxiliares de desenho
// ---------------------------------------------------------------------------
static void texto_direita(const UIFonte *fonte, const char *texto,
                          float direita, float y, float tamanho, Color cor) {
    if (texto == NULL) {
        return;
    }
    Vector2 medida = ui_medir_texto(fonte, texto, tamanho);
    ui_desenhar_texto(fonte, texto, (Vector2){direita - medida.x, y},
                      tamanho, cor);
}

static void desenhar_rotulo(const UIFonte *fonte, const char *texto,
                            Vector2 posicao, Color cor) {
    ui_desenhar_texto(fonte, texto, posicao, 10.5f, cor);
}

// ---------------------------------------------------------------------------
// Cenário
// ---------------------------------------------------------------------------
static void desenhar_fundo(const TelaDebate *tela) {
    if (tela->recursos != NULL && tela->recursos->fabrica.id != 0) {
        Rectangle area = {0.0f, 0.0f, LARGURA_TELA, ALTURA_TELA};
        recursos_desenhar_textura_cover(tela->recursos->fabrica, area);
    } else {
        ClearBackground(COR_CARVAO);
    }

    // Escurece o cenário para o clima de interrogação e para o texto respirar.
    DrawRectangle(0, 0, (int)LARGURA_TELA, (int)ALTURA_TELA,
                  (Color){27, 27, 24, 140});
    DrawRectangleGradientV(0, 0, (int)LARGURA_TELA, 150,
                           (Color){27, 27, 24, 175},
                           (Color){27, 27, 24, 0});
    DrawRectangleGradientV(0, 560, (int)LARGURA_TELA, 160,
                           (Color){27, 27, 24, 0},
                           (Color){27, 27, 24, 185});
}

static void desenhar_corpo_suspeito(const TelaDebate *tela, int id,
                                    const UIFonte *fonte_titulo) {
    const Suspeito *suspeito = buscar_suspeito(tela->jogo, id);
    if (suspeito == NULL) {
        return;
    }

    Vector2 base = posicao_suspeito(id);
    float altura = altura_suspeito(id);
    Texture2D textura = textura_suspeito(tela, id);

    if (textura.id != 0 && textura.height > 0) {
        float largura = altura * (float)textura.width /
                        (float)textura.height;
        Rectangle origem = {0.0f, 0.0f, (float)textura.width,
                            (float)textura.height};
        Rectangle destino = {base.x - largura / 2.0f, base.y - altura,
                             largura, altura};

        Color tonalidade = WHITE;
        if (suspeito->situacao == MORTO) {
            tonalidade = (Color){115, 109, 101, 200};
        } else if (suspeito->situacao == PRESO) {
            tonalidade = (Color){140, 133, 123, 220};
        }

        DrawTexturePro(textura, origem, destino, (Vector2){0.0f, 0.0f},
                       0.0f, tonalidade);
        return;
    }

    // Silhueta de emergência quando o sprite ainda não está disponível.
    float largura = fmaxf(altura * 0.34f, 84.0f);
    Rectangle corpo = {base.x - largura / 2.0f, base.y - altura, largura,
                       altura};
    Color fundo = COR_FUMACA;
    Color borda = COR_POEIRA;

    if (suspeito->situacao == MORTO) {
        fundo = COR_TINTA;
        borda = (Color){90, 86, 79, 255};
    } else if (suspeito->situacao == PRESO) {
        fundo = (Color){52, 51, 46, 255};
        borda = COR_POEIRA;
    }

    DrawRectangleRec(corpo, fundo);
    DrawRectangleLinesEx(corpo, 1.0f, borda);

    char inicial[2] = {
        nome_de(suspeito)[0],
        '\0',
    };
    ui_desenhar_texto_centralizado(fonte_titulo, inicial, corpo, 44.0f,
                                   COR_OSSO);
}

static void desenhar_mesa(void) {
    float centro_x = LARGURA_TELA / 2.0f;
    float centro_y = 568.0f;

    DrawEllipse((int)centro_x + 8, (int)centro_y + 12, 456.0f, 132.0f,
                COR_SOMBRA);
    DrawEllipse((int)centro_x, (int)centro_y, 450.0f, 128.0f, COR_FUMACA);
    DrawEllipseLines((int)centro_x, (int)centro_y, 450.0f, 128.0f,
                     COR_POEIRA);
    DrawEllipse((int)centro_x, (int)centro_y - 8, 408.0f, 104.0f, COR_TINTA);
    DrawEllipseLines((int)centro_x, (int)centro_y - 8, 408.0f, 104.0f,
                     (Color){138, 129, 117, 120});
}

// Processo em cima da mesa: um detalhe de arquivo de caso, sem competir com
// os retratos.
static void desenhar_dossie(void) {
    Rectangle pasta = {540.0f, 512.0f, 200.0f, 112.0f};
    DrawRectangleRec(pasta, COR_PAPEL);
    DrawRectangleLinesEx(pasta, 1.0f, COR_POEIRA);

    Rectangle aba = {540.0f, 500.0f, 92.0f, 14.0f};
    DrawRectangleRec(aba, COR_TIJOLO);

    for (int i = 0; i < 4; i++) {
        DrawRectangle(560, 536 + i * 17, 160, 1,
                      (Color){27, 27, 24, 90});
    }
    DrawRectangle(560, 598, 62, 4, COR_TIJOLO);
}

static void desenhar_situacao_suspeito(const TelaDebate *tela, int id) {
    const Suspeito *suspeito = buscar_suspeito(tela->jogo, id);
    if (suspeito == NULL || suspeito->situacao == VIVO) {
        return;
    }

    Vector2 base = posicao_suspeito(id);
    float topo = topo_suspeito(id);
    float raio = 19.0f;
    Vector2 centro = {base.x, topo + altura_suspeito(id) * 0.22f};

    Color cor = (suspeito->situacao == MORTO) ? COR_TIJOLO : COR_POEIRA;

    DrawLineEx((Vector2){centro.x - raio, centro.y - raio},
               (Vector2){centro.x + raio, centro.y + raio}, 3.0f, cor);
    DrawLineEx((Vector2){centro.x + raio, centro.y - raio},
               (Vector2){centro.x - raio, centro.y + raio}, 3.0f, cor);
}

static void desenhar_placa(const TelaDebate *tela, const UIFonte *fonte_corpo,
                           const UIFonte *fonte_corpo_bold, int id) {
    const Suspeito *suspeito = buscar_suspeito(tela->jogo, id);
    if (suspeito == NULL) {
        return;
    }

    const char *nome = nome_de(suspeito);
    char linha2[64];
    bool tem_linha2 = false;

    if (suspeito->situacao == MORTO) {
        snprintf(linha2, sizeof linha2, "MORTO");
        tem_linha2 = true;
    } else if (suspeito->situacao == PRESO) {
        snprintf(linha2, sizeof linha2, "PRESO");
        tem_linha2 = true;
    } else {
        const char *cargo = cargo_para_exibir(suspeito);
        if (cargo[0] != '\0') {
            snprintf(linha2, sizeof linha2, "%s", cargo);
            tem_linha2 = true;
        }
    }

    float largura_nome =
        ui_medir_texto(fonte_corpo_bold, nome, 13.0f).x;
    float largura_linha2 =
        tem_linha2 ? ui_medir_texto(fonte_corpo, linha2, 10.0f).x : 0.0f;
    float largura = fmaxf(152.0f,
                          fmaxf(largura_nome, largura_linha2) + 28.0f);
    float x = posicao_suspeito(id).x - largura / 2.0f;
    float y = topo_suspeito(id) - 78.0f;

    // Mantém a placa dentro da tela nos assentos das pontas.
    x = fmaxf(x, 8.0f);
    x = fminf(x, LARGURA_TELA - 8.0f - largura);

    bool selecionado = (id == alvo_indicado(tela));
    Color borda = selecionado ? COR_TIJOLO
                              : (suspeito->situacao == VIVO ? COR_POEIRA
                                                            : COR_FUMACA);

    Rectangle placa = {x, y, largura, 38.0f};
    DrawRectangleRec(placa, (Color){27, 27, 24, 236});
    DrawRectangleLinesEx(placa, selecionado ? 2.0f : 1.0f, borda);

    if (selecionado) {
        DrawRectangle((int)placa.x, (int)placa.y, 4, (int)placa.height,
                      COR_TIJOLO);
    }

    Color cor_nome = (suspeito->situacao == VIVO) ? COR_OSSO : COR_POEIRA;
    Color cor_linha2 = (suspeito->situacao == MORTO)
                           ? COR_TIJOLO
                           : (suspeito->situacao == PRESO ? COR_POEIRA
                                                         : COR_POEIRA);

    if (tem_linha2) {
        ui_desenhar_texto_centralizado(
            fonte_corpo_bold, nome,
            (Rectangle){placa.x, placa.y + 3.0f, placa.width, 20.0f},
            13.0f, cor_nome);
        ui_desenhar_texto_centralizado(
            fonte_corpo, linha2,
            (Rectangle){placa.x, placa.y + 21.0f, placa.width, 14.0f},
            10.0f, cor_linha2);
    } else {
        ui_desenhar_texto_centralizado(fonte_corpo_bold, nome, placa, 13.0f,
                                       cor_nome);
    }
}

static void desenhar_indicador(int id) {
    if (!assento_valido(id)) {
        return;
    }

    float x = posicao_suspeito(id).x;
    float topo = topo_suspeito(id);
    float ponta = topo - 6.0f;
    float base_triangulo = topo - 44.0f;

    DrawTriangle((Vector2){x, ponta - 8.0f},
                 (Vector2){x - 13.0f, base_triangulo},
                 (Vector2){x + 13.0f, base_triangulo},
                 (Color){194, 59, 34, 85});
    DrawTriangleLines((Vector2){x, ponta},
                      (Vector2){x - 18.0f, base_triangulo},
                      (Vector2){x + 18.0f, base_triangulo},
                      COR_TIJOLO);
}

// ---------------------------------------------------------------------------
// Cabeçalho
// ---------------------------------------------------------------------------
static void desenhar_cabecalho(const TelaDebate *tela,
                               const UIFonte *fonte_titulo,
                               const UIFonte *fonte_corpo,
                               const UIFonte *fonte_corpo_bold) {
    const Jogo *jogo = tela->jogo;

    const Depoimento *depoimento = NULL;
    if (tela->etapa == DEBATE_MOSTRANDO_DEPOIMENTO &&
        jogo->caderno != NULL) {
        depoimento = buscar_depoimento_por_id(jogo->caderno,
                                              tela->id_depoimento);
    }

    const Suspeito *falante = (depoimento != NULL)
                                  ? buscar_suspeito(jogo, depoimento->id_falante)
                                  : suspeito_do_turno(jogo);
    const char *rotulo_vez = (depoimento != NULL) ? "FALANDO" : "VEZ DE";

    DrawRectangle(0, 0, (int)LARGURA_TELA, (int)ALTURA_CABECALHO,
                  (Color){27, 27, 24, 238});
    DrawRectangle(0, (int)ALTURA_CABECALHO - 1, (int)LARGURA_TELA, 1,
                  (Color){138, 129, 117, 130});

    DrawRectangle((int)MARGEM, 18, 28, 2, COR_TIJOLO);
    desenhar_rotulo(fonte_corpo_bold, "INTERROGATÓRIO",
                    (Vector2){MARGEM, 26.0f}, COR_TIJOLO);
    ui_desenhar_texto(fonte_titulo, "DEBATE",
                      (Vector2){MARGEM, 38.0f}, 34.0f, COR_OSSO);

    char subtitulo[96];
    snprintf(subtitulo, sizeof subtitulo, "DIA %d/%d — %s",
             jogo->dia.dia_atual, TOTAL_DIAS,
             nome_fase(jogo->dia.fase_atual));
    ui_desenhar_texto(fonte_corpo, subtitulo,
                      (Vector2){MARGEM, 74.0f}, 11.5f, COR_POEIRA);

    // Bloco da vez: nome grande e cargo/estado em Courier.
    texto_direita(fonte_corpo_bold, rotulo_vez, 1080.0f, 24.0f, 10.5f,
                  COR_POEIRA);
    texto_direita(fonte_titulo,
                  (falante != NULL) ? nome_de(falante) : "—",
                  1080.0f, 38.0f, 23.0f, COR_OSSO);

    const char *cargo = (falante != NULL) ? cargo_para_exibir(falante) : "";
    if (cargo[0] == '\0') {
        cargo = (falante != NULL)
                    ? "SUSPEITO"
                    : (debate_encerrado(jogo) ? "RODA ENCERRADA"
                                              : "AGUARDANDO");
    }
    texto_direita(fonte_corpo, cargo, 1080.0f, 68.0f, 11.5f, COR_POEIRA);

    DrawRectangle(1104, 24, 1, 52, (Color){138, 129, 117, 90});

    char credibilidade[32];
    snprintf(credibilidade, sizeof credibilidade, "%d", jogo->credibilidade);
    texto_direita(fonte_corpo_bold, "CREDIBILIDADE", 1184.0f, 24.0f, 10.5f,
                  COR_POEIRA);
    texto_direita(fonte_titulo, credibilidade, 1184.0f, 38.0f, 24.0f,
                  COR_OSSO);
    texto_direita(fonte_corpo, "DE 100", 1184.0f, 68.0f, 11.5f, COR_POEIRA);
}

// ---------------------------------------------------------------------------
// Etapas
// ---------------------------------------------------------------------------
static void desenhar_prompt(const TelaDebate *tela,
                            const UIFonte *fonte_corpo,
                            const UIFonte *fonte_corpo_bold) {
    int id = alvo_indicado(tela);
    bool tem_turno = suspeito_do_turno(tela->jogo) != NULL;

    char texto[192];
    const char *dica = "SETAS  ALVO     ENTER  CONFIRMAR";

    if (debate_encerrado(tela->jogo)) {
        snprintf(texto, sizeof texto, "%s", TEXTO_RODA_ENCERRADA);
        dica = "ENTER  ENCERRAR";
    } else if (!tem_turno) {
        snprintf(texto, sizeof texto, "%s", TEXTO_SEM_TURNOS);
        dica = "ENCERRE O DEBATE";
    } else if (tela->etapa == DEBATE_ESCOLHENDO_PERIODO) {
        snprintf(texto, sizeof texto, "Em qual período interrogar %s?",
                 nome_por_id(tela->jogo, id));
        dica = "ESC  VOLTAR";
    } else {
        snprintf(texto, sizeof texto, "%s", TEXTO_SELECIONE);
    }

    Rectangle painel = {MARGEM, 106.0f, LARGURA_CONTEUDO, 64.0f};
    DrawRectangleRec(painel, (Color){27, 27, 24, 232});
    DrawRectangleLinesEx(painel, 1.0f, (Color){138, 129, 117, 120});
    DrawRectangle((int)painel.x, (int)painel.y, 3, (int)painel.height,
                  COR_TIJOLO);

    desenhar_rotulo(fonte_corpo_bold, "PRÓXIMO PASSO",
                    (Vector2){painel.x + 22.0f, painel.y + 10.0f}, COR_TIJOLO);
    ui_desenhar_texto(fonte_corpo, texto,
                      (Vector2){painel.x + 22.0f, painel.y + 28.0f},
                      17.0f, COR_OSSO);
    texto_direita(fonte_corpo, dica, painel.x + painel.width - 20.0f,
                  painel.y + 28.0f, 10.5f, COR_POEIRA);

    // Reforça o alvo atual no canto do painel, sem depender só da placa.
    if (tem_turno && id >= 0 &&
        tela->etapa == DEBATE_ESCOLHENDO_ALVO) {
        const Suspeito *alvo = buscar_suspeito(tela->jogo, id);
        if (alvo != NULL) {
            char resumo[128];
            const char *cargo = cargo_para_exibir(alvo);
            if (cargo[0] != '\0') {
                snprintf(resumo, sizeof resumo, "ALVO: %s · %s",
                         nome_de(alvo), cargo);
            } else {
                snprintf(resumo, sizeof resumo, "ALVO: %s", nome_de(alvo));
            }
            texto_direita(fonte_corpo_bold, resumo,
                          painel.x + painel.width - 20.0f,
                          painel.y + 10.0f, 10.5f, COR_POEIRA);
        }
    }
}

static void desenhar_periodo(const TelaDebate *tela,
                             const UIFonte *fonte_titulo,
                             const UIFonte *fonte_corpo,
                             const UIFonte *fonte_corpo_bold) {
    DrawRectangle(0, (int)ALTURA_CABECALHO, (int)LARGURA_TELA,
                  (int)(TOPO_BARRA_ACOES - ALTURA_CABECALHO),
                  (Color){27, 27, 24, 170});

    Rectangle painel = {400.0f, 286.0f, 480.0f, 160.0f};
    DrawRectangleRec(painel, (Color){27, 27, 24, 246});
    DrawRectangleLinesEx(painel, 1.0f, COR_POEIRA);
    DrawRectangle((int)painel.x, (int)painel.y, (int)painel.width, 3,
                  COR_TIJOLO);

    desenhar_rotulo(fonte_corpo_bold, "PERÍODO",
                    (Vector2){painel.x + 24.0f, painel.y + 18.0f},
                    COR_TIJOLO);
    ui_desenhar_texto(fonte_titulo, "Em qual período?",
                      (Vector2){painel.x + 24.0f, painel.y + 32.0f},
                      22.0f, COR_OSSO);
    ui_desenhar_texto(fonte_corpo, "A escolha vale só para esta pergunta.",
                      (Vector2){painel.x + 24.0f, painel.y + 64.0f},
                      11.0f, COR_POEIRA);

    for (int i = 0; i < TOTAL_PERIODOS; i++) {
        ui_desenhar_botao(fonte_corpo_bold,
                          (BotaoUI *)&tela->botoes_periodo[i],
                          tela->botao_focado == i);
    }

    ui_desenhar_botao(fonte_corpo_bold,
                      (BotaoUI *)&tela->botao_voltar_periodo, false);
}

static void desenhar_depoimento(const TelaDebate *tela,
                                const UIFonte *fonte_titulo,
                                const UIFonte *fonte_corpo,
                                const UIFonte *fonte_corpo_bold) {
    if (tela->jogo->caderno == NULL || tela->id_depoimento <= 0) {
        return;
    }

    Depoimento *depoimento = buscar_depoimento_por_id(
        tela->jogo->caderno, tela->id_depoimento);
    if (depoimento == NULL) {
        return;
    }

    DrawRectangle(0, (int)ALTURA_CABECALHO, (int)LARGURA_TELA,
                  (int)(TOPO_BARRA_ACOES - ALTURA_CABECALHO),
                  (Color){27, 27, 24, 185});

    Rectangle esquerda = {MARGEM, 250.0f, 760.0f, 370.0f};
    Rectangle direita = {880.0f, 250.0f, 304.0f, 370.0f};

    DrawRectangleRec(esquerda, (Color){27, 27, 24, 246});
    DrawRectangleLinesEx(esquerda, 1.0f, COR_POEIRA);
    DrawRectangle((int)esquerda.x, (int)esquerda.y, (int)esquerda.width, 4,
                  COR_TIJOLO);

    DrawRectangleRec(direita, (Color){27, 27, 24, 240});
    DrawRectangleLinesEx(direita, 1.0f, COR_POEIRA);
    DrawRectangle((int)direita.x, (int)direita.y, (int)direita.width, 4,
                  COR_POEIRA);

    desenhar_rotulo(fonte_corpo_bold, "DEPOIMENTO",
                    (Vector2){esquerda.x + 24.0f, esquerda.y + 18.0f},
                    COR_TIJOLO);
    ui_desenhar_texto(fonte_titulo, nome_por_id(tela->jogo,
                                                depoimento->id_falante),
                      (Vector2){esquerda.x + 24.0f, esquerda.y + 32.0f},
                      30.0f, COR_OSSO);

    char meta[320];
    snprintf(meta, sizeof meta, "Dia %d — %s · sobre %s · local: %s",
             depoimento->dia, nome_periodo(depoimento->periodo),
             nome_por_id(tela->jogo, depoimento->id_assunto),
             (depoimento->local != NULL) ? depoimento->local : "—");
    ui_desenhar_texto(fonte_corpo, meta,
                      (Vector2){esquerda.x + 24.0f, esquerda.y + 70.0f},
                      12.5f, COR_POEIRA);

    DrawRectangle((int)esquerda.x + 24, (int)esquerda.y + 94,
                  (int)esquerda.width - 48, 1,
                  (Color){138, 129, 117, 100});

    Rectangle area_texto = {
        esquerda.x + 24.0f,
        esquerda.y + 108.0f,
        esquerda.width - 48.0f,
        170.0f,
    };
    BeginScissorMode((int)area_texto.x, (int)area_texto.y,
                     (int)area_texto.width, (int)area_texto.height);
    ui_desenhar_texto_quebrado(
        fonte_corpo,
        (depoimento->texto != NULL && depoimento->texto[0] != '\0')
            ? depoimento->texto
            : TEXTO_DEPOIMENTO_VAZIO,
        area_texto, 17.0f, 5.0f, COR_OSSO);
    EndScissorMode();

    if (tela->tempo_alerta > 0.0f) {
        Rectangle alerta = {esquerda.x + 24.0f, esquerda.y + 298.0f,
                            esquerda.width - 48.0f, 42.0f};
        DrawRectangleRec(alerta, COR_RUBOR);
        DrawRectangle((int)alerta.x, (int)alerta.y, 3, (int)alerta.height,
                      COR_TIJOLO);
        ui_desenhar_icone(UI_ICONE_ALERTA,
                          (Vector2){alerta.x + 22.0f,
                                    alerta.y + alerta.height / 2.0f},
                          18.0f, COR_TIJOLO);
        ui_desenhar_texto(fonte_corpo, TEXTO_NOVA_CONTRADICAO,
                          (Vector2){alerta.x + 40.0f, alerta.y + 13.0f},
                          14.0f, COR_OSSO);
    } else {
        ui_desenhar_texto(fonte_corpo,
                          "ENTER ou CONTINUAR para voltar ao debate.",
                          (Vector2){esquerda.x + 24.0f,
                                    esquerda.y + 306.0f},
                          11.5f, COR_POEIRA);
    }

    desenhar_rotulo(fonte_corpo_bold, "REAÇÕES",
                    (Vector2){direita.x + 24.0f, direita.y + 18.0f},
                    COR_POEIRA);

    int reacoes = 0;
    for (NoListaDupla *no = tela->jogo->caderno->reacoes.inicio;
         no != NULL; no = no->prox) {
        const Reacao *reacao = (const Reacao *)no->dado;
        if (reacao == NULL ||
            reacao->id_depoimento != tela->id_depoimento) {
            continue;
        }
        reacoes++;
    }

    char total[16];
    snprintf(total, sizeof total, "%d", reacoes);
    texto_direita(fonte_titulo, total, direita.x + direita.width - 24.0f,
                  direita.y + 14.0f, 20.0f, COR_OSSO);

    DrawRectangle((int)direita.x + 24, (int)direita.y + 48,
                  (int)direita.width - 48, 1,
                  (Color){138, 129, 117, 100});

    int indice = 0;
    for (NoListaDupla *no = tela->jogo->caderno->reacoes.inicio;
         no != NULL && indice < 7; no = no->prox) {
        const Reacao *reacao = (const Reacao *)no->dado;
        if (reacao == NULL ||
            reacao->id_depoimento != tela->id_depoimento) {
            continue;
        }

        float y = direita.y + 64.0f + (float)indice * 54.0f;
        Color cor = cor_da_reacao(reacao->tipo_reacao);

        ui_desenhar_icone(icone_da_reacao(reacao->tipo_reacao),
                          (Vector2){direita.x + 40.0f, y + 15.0f},
                          18.0f, cor);

        const char *quem = (reacao->quem_reagiu != NULL)
                               ? reacao->quem_reagiu
                               : "?";
        ui_desenhar_texto(fonte_corpo_bold, quem,
                          (Vector2){direita.x + 60.0f, y},
                          13.5f, COR_OSSO);
        desenhar_rotulo(fonte_corpo_bold,
                        rotulo_da_reacao(reacao->tipo_reacao),
                        (Vector2){direita.x + 60.0f, y + 18.0f},
                        COR_POEIRA);

        indice++;
    }

    if (reacoes == 0) {
        ui_desenhar_texto(fonte_corpo, TEXTO_SEM_REACOES,
                          (Vector2){direita.x + 24.0f, direita.y + 68.0f},
                          13.5f, COR_POEIRA);
        ui_desenhar_texto(fonte_corpo,
                          "O depoimento não mexeu com a roda.",
                          (Vector2){direita.x + 24.0f, direita.y + 90.0f},
                          10.5f, COR_POEIRA);
    }
}

static void desenhar_barra_acoes(const TelaDebate *tela,
                                 const UIFonte *fonte_corpo_bold) {
    DrawRectangle(0, (int)TOPO_BARRA_ACOES, (int)LARGURA_TELA,
                  (int)ALTURA_BARRA_ACOES, (Color){27, 27, 24, 242});
    DrawRectangle(0, (int)TOPO_BARRA_ACOES, (int)LARGURA_TELA, 1,
                  (Color){138, 129, 117, 120});

    ui_desenhar_botao(fonte_corpo_bold,
                      (BotaoUI *)&tela->botao_caderno, false);
    ui_desenhar_botao(fonte_corpo_bold,
                      (BotaoUI *)&tela->botao_interrogar, false);
    ui_desenhar_botao(fonte_corpo_bold,
                      (BotaoUI *)&tela->botao_encerrar,
                      debate_encerrado(tela->jogo));
}

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------
void tela_debate_iniciar(TelaDebate *tela, Jogo *jogo,
                         const Recursos *recursos) {
    if (tela == NULL) {
        return;
    }

    memset(tela, 0, sizeof *tela);
    tela->jogo = jogo;
    tela->recursos = recursos;
    tela->etapa = DEBATE_ESCOLHENDO_ALVO;
    tela->id_alvo = primeiro_vivo(jogo);
    tela->id_depoimento = -1;
    tela->tempo_alerta = 0.0f;
    tela->botao_focado = -1;

    tela->botao_caderno = (BotaoUI){
        .area = {MARGEM, 650.0f, 240.0f, 56.0f},
        .rotulo = "ABRIR CADERNO",
        .habilitado = true,
        .primario = false,
    };
    tela->botao_interrogar = (BotaoUI){
        .area = {520.0f, 650.0f, 240.0f, 56.0f},
        .rotulo = "INTERROGAR",
        .habilitado = true,
        .primario = true,
    };
    tela->botao_encerrar = (BotaoUI){
        .area = {944.0f, 650.0f, 240.0f, 56.0f},
        .rotulo = "ENCERRAR DEBATE",
        .habilitado = true,
        .primario = false,
    };

    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        tela->botoes_alvo[id] = (BotaoUI){
            .area = area_suspeito(tela, id),
            .rotulo = NULL,
            .habilitado = true,
            .primario = false,
        };
    }

    for (int i = 0; i < TOTAL_PERIODOS; i++) {
        tela->botoes_periodo[i] = (BotaoUI){
            .area = {418.0f + (float)i * 152.0f, 366.0f, 140.0f, 44.0f},
            .rotulo = ROTULOS_PERIODO[i],
            .habilitado = true,
            .primario = false,
        };
    }

    tela->botao_voltar_periodo = (BotaoUI){
        .area = {580.0f, 416.0f, 120.0f, 26.0f},
        .rotulo = "VOLTAR",
        .habilitado = true,
        .primario = false,
    };
}

static EntradaBotao ler_entrada(void) {
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

AcaoDebate tela_debate_atualizar(TelaDebate *tela) {
    if (tela == NULL || tela->jogo == NULL) {
        return TELA_DEBATE_CONTINUAR;
    }

    Jogo *jogo = tela->jogo;
    EntradaBotao entrada = ler_entrada();

    if (tela->tempo_alerta > 0.0f) {
        tela->tempo_alerta -= GetFrameTime();
        if (tela->tempo_alerta < 0.0f) {
            tela->tempo_alerta = 0.0f;
        }
    }

    Suspeito *falante = suspeito_do_turno(jogo);
    bool pode_interrogar =
        falante != NULL && jogo->resultado == JOGO_EM_ANDAMENTO;

    // Mantém os alvos sincronizados antes da navegação por teclado.
    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        tela->botoes_alvo[id].habilitado =
            pode_interrogar &&
            tela->etapa == DEBATE_ESCOLHENDO_ALVO &&
            suspeito_vivo(jogo, id) != NULL;
    }

    if (tela->etapa == DEBATE_ESCOLHENDO_ALVO && pode_interrogar) {
        int passo = 0;
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_DOWN) ||
            IsKeyPressed(KEY_TAB)) {
            passo = 1;
        } else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_UP)) {
            passo = -1;
        }

        if (passo != 0) {
            int proximo = ui_navegar_botoes(
                tela->botoes_alvo, TOTAL_SUSPEITOS, tela->id_alvo, passo);
            if (proximo >= 0) {
                tela->id_alvo = proximo;
            }
        }

        if (entrada.confirmar && suspeito_vivo(jogo, tela->id_alvo) != NULL) {
            tela->etapa = DEBATE_ESCOLHENDO_PERIODO;
            tela->botao_focado = 0;
            entrada.confirmar = false;
        }
    } else if (tela->etapa == DEBATE_ESCOLHENDO_PERIODO &&
               IsKeyPressed(KEY_ESCAPE)) {
        tela->etapa = DEBATE_ESCOLHENDO_ALVO;
        tela->botao_focado = -1;
    } else if (tela->etapa == DEBATE_MOSTRANDO_DEPOIMENTO &&
               (entrada.confirmar || IsKeyPressed(KEY_ESCAPE))) {
        tela->etapa = DEBATE_ESCOLHENDO_ALVO;
        tela->id_depoimento = -1;
        tela->botao_focado = -1;
        entrada.confirmar = false;
    }

    // Com a roda encerrada, Enter encerra o debate direto, como o botão.
    if (debate_encerrado(jogo) && entrada.confirmar &&
        tela->etapa == DEBATE_ESCOLHENDO_ALVO) {
        return TELA_DEBATE_ENCERRAR;
    }

    // Alvos: só são clicáveis quando é a vez de escolher.
    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        EstadoBotao estado = ui_atualizar_botao(
            &tela->botoes_alvo[id], false, &entrada);

        if (tela->etapa == DEBATE_ESCOLHENDO_ALVO && pode_interrogar) {
            if (estado == BOTAO_HOVER) {
                tela->id_alvo = id;
            } else if (estado == BOTAO_ACIONADO) {
                tela->id_alvo = id;
                tela->etapa = DEBATE_ESCOLHENDO_PERIODO;
                tela->botao_focado = 0;
            }
        }
    }

    // Períodos: visíveis apenas depois de escolher o alvo.
    bool pode_periodo =
        pode_interrogar && tela->etapa == DEBATE_ESCOLHENDO_PERIODO;

    if (pode_periodo) {
        int foco = (tela->botao_focado >= 0 &&
                    tela->botao_focado < TOTAL_PERIODOS)
                       ? tela->botao_focado
                       : 0;

        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_DOWN) ||
            IsKeyPressed(KEY_TAB)) {
            foco = (foco + 1) % TOTAL_PERIODOS;
        } else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_UP)) {
            foco = (foco + TOTAL_PERIODOS - 1) % TOTAL_PERIODOS;
        }

        for (int i = 0; i < TOTAL_PERIODOS; i++) {
            if (CheckCollisionPointRec(entrada.mouse,
                                       tela->botoes_periodo[i].area)) {
                foco = i;
            }
        }

        tela->botao_focado = foco;
    }

    for (int i = 0; i < TOTAL_PERIODOS; i++) {
        tela->botoes_periodo[i].habilitado = pode_periodo;

        EstadoBotao estado = ui_atualizar_botao(
            &tela->botoes_periodo[i],
            i == tela->botao_focado, &entrada);
        if (pode_periodo && estado == BOTAO_ACIONADO) {
            int antes = (jogo->caderno != NULL)
                            ? jogo->caderno->contradicoes.tamanho
                            : 0;
            int id_depoimento = perguntar_sobre(
                jogo, tela->id_alvo, (enum Periodo)i);

            tela->botao_focado = 0;
            if (id_depoimento > 0) {
                tela->id_depoimento = id_depoimento;
                tela->etapa = DEBATE_MOSTRANDO_DEPOIMENTO;
                tela->tempo_alerta =
                    (jogo->caderno != NULL &&
                     jogo->caderno->contradicoes.tamanho > antes)
                        ? 4.0f
                        : 0.0f;
            } else {
                tela->etapa = DEBATE_ESCOLHENDO_ALVO;
            }
        }
    }

    tela->botao_voltar_periodo.habilitado = pode_periodo;
    if (pode_periodo &&
        ui_atualizar_botao(&tela->botao_voltar_periodo, false, &entrada) ==
            BOTAO_ACIONADO) {
        tela->etapa = DEBATE_ESCOLHENDO_ALVO;
        tela->botao_focado = -1;
    }

    // Botões de ação.
    bool encerrada = debate_encerrado(jogo) || !pode_interrogar;

    if (tela->etapa == DEBATE_ESCOLHENDO_ALVO) {
        tela->botao_interrogar.rotulo = "INTERROGAR";
        tela->botao_interrogar.habilitado =
            pode_interrogar && suspeito_vivo(jogo, tela->id_alvo) != NULL;
    } else if (tela->etapa == DEBATE_ESCOLHENDO_PERIODO) {
        tela->botao_interrogar.rotulo = "INTERROGAR";
        tela->botao_interrogar.habilitado = false;
    } else {
        tela->botao_interrogar.rotulo = "CONTINUAR";
        tela->botao_interrogar.habilitado = true;
    }
    tela->botao_interrogar.primario = !encerrada;

    if (ui_atualizar_botao(&tela->botao_interrogar, false, &entrada) ==
        BOTAO_ACIONADO) {
        if (tela->etapa == DEBATE_ESCOLHENDO_ALVO) {
            if (pode_interrogar &&
                suspeito_vivo(jogo, tela->id_alvo) != NULL) {
                tela->etapa = DEBATE_ESCOLHENDO_PERIODO;
                tela->botao_focado = 0;
            }
        } else if (tela->etapa == DEBATE_MOSTRANDO_DEPOIMENTO) {
            tela->etapa = DEBATE_ESCOLHENDO_ALVO;
            tela->id_depoimento = -1;
            tela->botao_focado = -1;
        }
    }

    // Atalhos de teclado para as ações fixas (o mouse também funciona).
    tela->botao_caderno.habilitado = true;
    if (ui_atualizar_botao(&tela->botao_caderno, false, &entrada) ==
            BOTAO_ACIONADO ||
        IsKeyPressed(KEY_C)) {
        return TELA_DEBATE_ABRIR_CADERNO;
    }

    tela->botao_encerrar.habilitado = true;
    tela->botao_encerrar.primario = encerrada;
    if (ui_atualizar_botao(&tela->botao_encerrar, false, &entrada) ==
            BOTAO_ACIONADO ||
        IsKeyPressed(KEY_E)) {
        return TELA_DEBATE_ENCERRAR;
    }

    return TELA_DEBATE_CONTINUAR;
}

void tela_debate_desenhar(const TelaDebate *tela,
                          const UIFonte *fonte_titulo,
                          const UIFonte *fonte_corpo,
                          const UIFonte *fonte_corpo_bold) {
    if (tela == NULL || tela->jogo == NULL) {
        return;
    }

    desenhar_fundo(tela);

    // Corpos primeiro; a mesa cobre a parte de baixo de quem está atrás dela.
    static const int ORDEM_DESENHO[TOTAL_SUSPEITOS] = {2, 3, 1, 4, 0, 5};
    for (int i = 0; i < TOTAL_SUSPEITOS; i++) {
        desenhar_corpo_suspeito(tela, ORDEM_DESENHO[i], fonte_titulo);
    }

    desenhar_mesa();
    desenhar_dossie();

    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        desenhar_situacao_suspeito(tela, id);
    }

    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        desenhar_placa(tela, fonte_corpo, fonte_corpo_bold, id);
    }

    int indicado = alvo_indicado(tela);
    if (indicado >= 0) {
        desenhar_indicador(indicado);
    }

    desenhar_cabecalho(tela, fonte_titulo, fonte_corpo, fonte_corpo_bold);

    if (tela->etapa == DEBATE_MOSTRANDO_DEPOIMENTO) {
        desenhar_depoimento(tela, fonte_titulo, fonte_corpo,
                            fonte_corpo_bold);
    } else if (tela->etapa == DEBATE_ESCOLHENDO_PERIODO) {
        desenhar_periodo(tela, fonte_titulo, fonte_corpo, fonte_corpo_bold);
        desenhar_prompt(tela, fonte_corpo, fonte_corpo_bold);
    } else {
        desenhar_prompt(tela, fonte_corpo, fonte_corpo_bold);
    }

    desenhar_barra_acoes(tela, fonte_corpo_bold);
}
