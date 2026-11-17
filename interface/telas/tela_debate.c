#include "interface/telas/tela_debate.h"
#include "interface/ui.h"
#include "raylib.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
 * Tela do debate — versão provisória/template.
 *
 * O cenário segue o rascunho do time: uma mesa redonda no centro, os suspeitos
 * ao redor dela e o detetive em primeira pessoa (não desenhado). O triângulo
 * vermelho indica o alvo selecionado. Os sprites e o fundo são placeholders:
 * se os arquivos não existirem, a tela cai em formas desenhadas.
 */

#define LARGURA_TELA 960.0f
#define ALTURA_TELA 540.0f
#define ALTURA_CABECALHO 76.0f
#define ALTURA_BARRA_ACOES 58.0f

#define DEBATE_FUNDO "assets/image/fabrica.png"

static const char *RETRATOS[TOTAL_SUSPEITOS] = {
    [ID_GERENTE]    = "assets/image/gerente.png",
    [ID_CAPATAZ]    = "assets/image/capataz.png",
    [ID_ENGENHEIRO] = "assets/image/engenheiro.png",
    [ID_OPERARIO]   = "assets/image/operario.png",
    [ID_ENFERMEIRA] = "assets/image/enfermeira.png",
    [ID_VIGIA]      = "assets/image/vigia.png",
};

static const char *ROTULOS_PERIODO[TOTAL_PERIODOS] = {
    [MANHA] = "Manhã",
    [TARDE] = "Tarde",
    [NOITE] = "Noite",
};

static const char *TEXTO_SELECIONE =
    "Selecione quem você quer interrogar";
static const char *TEXTO_SEM_TURNOS =
    "Todos os suspeitos vivos já falaram hoje.";
static const char *TEXTO_NOVA_CONTRADICAO =
    "Nova contradição registrada no caderno!";
static const char *TEXTO_RODA_ENCERRADA =
    "Todos os vivos falaram. Encerre o debate para ir à noite.";

static const Color COR_PAINEL = {18, 18, 28, 235};
static const Color COR_BORDA = {100, 100, 128, 255};
static const Color COR_TEXTO_SUAVE = {190, 190, 205, 255};
static const Color COR_DESTAQUE = {245, 205, 70, 255};
static const Color COR_ALERTA = {240, 90, 90, 255};
static const Color COR_CONCORDA = {120, 220, 150, 255};
static const Color COR_DISCORDA = {235, 105, 105, 255};
static const Color COR_NEUTRO = {175, 175, 195, 255};

// ---------------------------------------------------------------------------
// Assets
// ---------------------------------------------------------------------------
static Texture2D carregar_textura(const char *caminho) {
    if (caminho == NULL) {
        return (Texture2D){0};
    }

    if (FileExists(caminho)) {
        return LoadTexture(caminho);
    }

    const char *pasta = GetApplicationDirectory();
    char tentativa[1024];

    if (pasta != NULL) {
        snprintf(tentativa, sizeof tentativa, "%s%s", pasta, caminho);
        if (FileExists(tentativa)) {
            return LoadTexture(tentativa);
        }
    }

    snprintf(tentativa, sizeof tentativa, "../%s", caminho);
    if (FileExists(tentativa)) {
        return LoadTexture(tentativa);
    }

    snprintf(tentativa, sizeof tentativa, "../../%s", caminho);
    if (FileExists(tentativa)) {
        return LoadTexture(tentativa);
    }

    return (Texture2D){0};
}

void tela_debate_liberar(TelaDebate *tela) {
    if (tela == NULL) {
        return;
    }

    if (tela->fundo.id != 0) {
        UnloadTexture(tela->fundo);
        tela->fundo = (Texture2D){0};
    }

    for (int i = 0; i < TOTAL_SUSPEITOS; i++) {
        if (tela->retratos[i].id != 0) {
            UnloadTexture(tela->retratos[i]);
            tela->retratos[i] = (Texture2D){0};
        }
    }
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------
// Assentos fixos: os suspeitos formam um arco em volta da mesa. "base" é o
// ponto em que os pés tocam o chão; como a mesa é desenhada depois, ela
// esconde as pernas de quem está do outro lado dela.
typedef struct {
    Vector2 base;
    float altura;
} AssentoSuspeito;

static const AssentoSuspeito ASSENTOS[TOTAL_SUSPEITOS] = {
    [ID_GERENTE]    = {{96.0f, 396.0f}, 196.0f},  // frente esquerda
    [ID_CAPATAZ]    = {{224.0f, 372.0f}, 168.0f},
    [ID_ENGENHEIRO] = {{384.0f, 350.0f}, 152.0f}, // fundo esquerda
    [ID_OPERARIO]   = {{576.0f, 350.0f}, 152.0f}, // fundo direita
    [ID_ENFERMEIRA] = {{736.0f, 372.0f}, 168.0f},
    [ID_VIGIA]      = {{874.0f, 396.0f}, 196.0f}, // frente direita
};

static bool assento_valido(int id) {
    return id >= 0 && id < TOTAL_SUSPEITOS;
}

static Vector2 posicao_suspeito(int id) {
    if (!assento_valido(id)) {
        return (Vector2){LARGURA_TELA / 2.0f, 372.0f};
    }
    return ASSENTOS[id].base;
}

static float altura_suspeito(int id) {
    if (!assento_valido(id)) {
        return 152.0f;
    }
    return ASSENTOS[id].altura;
}

static float largura_suspeito(const TelaDebate *tela, int id) {
    float altura = altura_suspeito(id);
    float aspecto = 0.78f;

    if (tela != NULL && assento_valido(id) &&
        tela->retratos[id].id != 0 && tela->retratos[id].height > 0) {
        aspecto = (float)tela->retratos[id].width /
                  (float)tela->retratos[id].height;
    }

    return altura * aspecto;
}

static Rectangle area_suspeito(const TelaDebate *tela, int id) {
    Vector2 base = posicao_suspeito(id);
    float altura = altura_suspeito(id);
    float largura = largura_suspeito(tela, id);

    if (largura < 96.0f) {
        largura = 96.0f;
    }

    return (Rectangle){
        base.x - largura / 2.0f,
        base.y - altura - 46.0f, // margem para o triângulo de seleção
        largura,
        altura + 52.0f
    };
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
    if (tela == NULL || tela->etapa == DEBATE_MOSTRANDO_DEPOIMENTO ||
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

static const char *nome_suspeito(const Jogo *jogo, int id) {
    const Suspeito *suspeito = buscar_suspeito(jogo, id);
    return (suspeito != NULL) ? suspeito->nome : "desconhecido";
}

static UIIcone icone_da_reacao(enum TipoReacao tipo) {
    switch (tipo) {
        case CONCORDA: return UI_ICONE_CONCORDA;
        case DISCORDA: return UI_ICONE_DISCORDA;
        default:       return UI_ICONE_NEUTRO;
    }
}

static Color cor_da_reacao(enum TipoReacao tipo) {
    switch (tipo) {
        case CONCORDA: return COR_CONCORDA;
        case DISCORDA: return COR_DISCORDA;
        default:       return COR_NEUTRO;
    }
}

// ---------------------------------------------------------------------------
// Desenho do cenário
// ---------------------------------------------------------------------------
static void desenhar_fundo(const TelaDebate *tela) {
    DrawRectangle(0, 0, (int)LARGURA_TELA, (int)ALTURA_TELA,
                  (Color){20, 20, 28, 255});

    if (tela->fundo.id != 0) {
        float largura = LARGURA_TELA;
        float altura = ALTURA_TELA;
        float escala_x = LARGURA_TELA / (float)tela->fundo.width;
        float escala_y = ALTURA_TELA / (float)tela->fundo.height;
        float escala = (escala_x > escala_y) ? escala_x : escala_y;

        largura = (float)tela->fundo.width * escala;
        altura = (float)tela->fundo.height * escala;

        Rectangle origem = {0.0f, 0.0f,
                            (float)tela->fundo.width,
                            (float)tela->fundo.height};
        Rectangle destino = {
            (LARGURA_TELA - largura) / 2.0f,
            (ALTURA_TELA - altura) / 2.0f,
            largura,
            altura
        };
        DrawTexturePro(tela->fundo, origem, destino, (Vector2){0.0f, 0.0f},
                       0.0f, (Color){135, 135, 150, 255});
    }

    // Escurece o cenário para dar o clima de interrogação e destacar a mesa.
    DrawRectangle(0, 0, (int)LARGURA_TELA, (int)ALTURA_TELA,
                  (Color){8, 10, 18, 155});
}

static void desenhar_corpo_suspeito(const TelaDebate *tela, int id) {
    const Suspeito *suspeito = buscar_suspeito(tela->jogo, id);
    if (suspeito == NULL) {
        return;
    }

    Vector2 posicao = posicao_suspeito(id);
    float altura = altura_suspeito(id);
    float base = posicao.y;
    Texture2D textura = tela->retratos[id];

    if (textura.id != 0) {
        float largura = altura * (float)textura.width /
                        (float)textura.height;
        Rectangle origem = {0.0f, 0.0f,
                            (float)textura.width, (float)textura.height};
        Rectangle destino = {posicao.x - largura / 2.0f, base - altura,
                             largura, altura};

        Color tonalidade = WHITE;
        if (suspeito->situacao == MORTO) {
            tonalidade = (Color){70, 70, 82, 190};
        } else if (suspeito->situacao == PRESO) {
            tonalidade = (Color){110, 110, 128, 210};
        }

        DrawTexturePro(textura, origem, destino, (Vector2){0.0f, 0.0f},
                       0.0f, tonalidade);
        return;
    }

    // Placeholder quando o sprite ainda não está disponível.
    Rectangle corpo = {posicao.x - 36.0f, base - altura, 72.0f, altura};
    Color fundo = (Color){55, 55, 72, 255};
    Color borda = (Color){120, 120, 145, 255};

    if (suspeito->situacao == MORTO) {
        fundo = (Color){35, 35, 42, 255};
        borda = (Color){85, 85, 95, 255};
    } else if (suspeito->situacao == PRESO) {
        fundo = (Color){45, 45, 55, 255};
        borda = (Color){100, 100, 120, 255};
    }

    DrawRectangleRec(corpo, fundo);
    DrawRectangleLinesEx(corpo, 2.0f, borda);

    char inicial[2] = {
        (suspeito->nome != NULL && suspeito->nome[0] != '\0')
            ? suspeito->nome[0]
            : '?',
        '\0'
    };
    Rectangle area_inicial = {corpo.x, corpo.y, corpo.width, corpo.height};
    ui_desenhar_texto_centralizado(NULL, inicial, area_inicial, 34.0f,
                                   (Color){220, 220, 235, 255});
}

static void desenhar_mesa(void) {
    DrawEllipse((int)(LARGURA_TELA / 2.0f) + 6, 416, 320, 102,
                (Color){0, 0, 0, 105});
    DrawEllipse((int)(LARGURA_TELA / 2.0f), 408, 320, 102,
                (Color){28, 63, 44, 255});
    DrawEllipseLines((int)(LARGURA_TELA / 2.0f), 408, 320, 102,
                     (Color){78, 175, 112, 255});
    DrawEllipse((int)(LARGURA_TELA / 2.0f), 402, 280, 80,
                (Color){22, 52, 37, 255});
    DrawEllipseLines((int)(LARGURA_TELA / 2.0f), 402, 280, 80,
                     (Color){56, 128, 84, 180});
}

static void desenhar_situacao_suspeito(const TelaDebate *tela, int id) {
    const Suspeito *suspeito = buscar_suspeito(tela->jogo, id);
    if (suspeito == NULL || suspeito->situacao == VIVO) {
        return;
    }

    Vector2 posicao = posicao_suspeito(id);
    float altura = altura_suspeito(id);
    float base = posicao.y;
    float topo = base - altura;

    Color cor = (suspeito->situacao == MORTO)
                    ? (Color){220, 65, 65, 235}
                    : (Color){150, 150, 175, 235};
    float raio = 22.0f;
    Vector2 centro = {posicao.x, topo + altura * 0.28f};

    DrawLineEx((Vector2){centro.x - raio, centro.y - raio},
               (Vector2){centro.x + raio, centro.y + raio}, 4.0f, cor);
    DrawLineEx((Vector2){centro.x + raio, centro.y - raio},
               (Vector2){centro.x - raio, centro.y + raio}, 4.0f, cor);

    const char *rotulo = (suspeito->situacao == MORTO) ? "MORTO" : "PRESO";
    Rectangle area = {posicao.x - 60.0f, topo - 24.0f, 120.0f, 20.0f};
    ui_desenhar_texto_centralizado(NULL, rotulo, area, 15.0f, cor);
}

static void desenhar_indicador(const TelaDebate *tela,
                               const UIFonte *fonte, int id) {
    const Suspeito *suspeito = buscar_suspeito(tela->jogo, id);
    if (suspeito == NULL) {
        return;
    }

    Vector2 posicao = posicao_suspeito(id);
    float altura = altura_suspeito(id);
    float topo = posicao.y - altura;

    float ponta = topo - 6.0f;
    float base_triangulo = topo - 38.0f;

    DrawTriangle((Vector2){posicao.x, ponta - 6.0f},
                 (Vector2){posicao.x - 11.0f, base_triangulo},
                 (Vector2){posicao.x + 11.0f, base_triangulo},
                 (Color){220, 40, 40, 90});
    DrawTriangleLines((Vector2){posicao.x, ponta},
                      (Vector2){posicao.x - 17.0f, base_triangulo},
                      (Vector2){posicao.x + 17.0f, base_triangulo},
                      (Color){235, 45, 45, 255});

    char rotulo[128];
    snprintf(rotulo, sizeof rotulo, "%s — %s",
             (suspeito->nome != NULL) ? suspeito->nome : "?",
             (suspeito->cargo != NULL) ? suspeito->cargo : "?");

    Vector2 medida = ui_medir_texto(fonte, rotulo, 15.0f);
    float largura_placa = medida.x + 20.0f;
    float x_placa = posicao.x - largura_placa / 2.0f;

    // Mantém a placa dentro da tela quando o alvo está nas pontas.
    if (x_placa < 8.0f) {
        x_placa = 8.0f;
    }
    if (x_placa + largura_placa > LARGURA_TELA - 8.0f) {
        x_placa = LARGURA_TELA - 8.0f - largura_placa;
    }

    Rectangle placa = {
        x_placa,
        base_triangulo - 30.0f,
        largura_placa,
        24.0f
    };

    DrawRectangleRec(placa, (Color){16, 16, 24, 225});
    DrawRectangleLinesEx(placa, 1.0f, (Color){235, 45, 45, 255});
    ui_desenhar_texto_centralizado(fonte, rotulo, placa, 15.0f, RAYWHITE);
}

static void desenhar_cabecalho(const TelaDebate *tela,
                               const UIFonte *fonte) {
    const Jogo *jogo = tela->jogo;

    // Durante o depoimento o painel está na frente; o cabeçalho passa a
    // mostrar quem está falando, e não o próximo da roda.
    const Depoimento *depoimento = NULL;
    if (tela->etapa == DEBATE_MOSTRANDO_DEPOIMENTO &&
        jogo->caderno != NULL) {
        depoimento = buscar_depoimento_por_id(jogo->caderno,
                                              tela->id_depoimento);
    }

    const Suspeito *falante = (depoimento != NULL)
                                  ? buscar_suspeito(jogo, depoimento->id_falante)
                                  : suspeito_do_turno(jogo);

    DrawRectangle(0, 0, (int)LARGURA_TELA, (int)ALTURA_CABECALHO,
                  (Color){10, 10, 16, 225});
    DrawRectangle(0, (int)ALTURA_CABECALHO - 2, (int)LARGURA_TELA, 2,
                  (Color){86, 86, 112, 220});

    ui_desenhar_texto(fonte, "DEBATE", (Vector2){24.0f, 10.0f}, 32.0f,
                      RAYWHITE);

    char subtitulo[96];
    snprintf(subtitulo, sizeof subtitulo, "Dia %d — %s",
             jogo->dia.dia_atual, nome_fase(jogo->dia.fase_atual));
    ui_desenhar_texto(fonte, subtitulo, (Vector2){24.0f, 48.0f}, 17.0f,
                      COR_TEXTO_SUAVE);

    char vez[160];
    if (falante != NULL) {
        snprintf(vez, sizeof vez, "%s %s — %s",
                 (depoimento != NULL) ? "Falando:" : "Vez de",
                 (falante->nome != NULL) ? falante->nome : "?",
                 (falante->cargo != NULL) ? falante->cargo : "?");
    } else {
        snprintf(vez, sizeof vez, "%s", TEXTO_SEM_TURNOS);
    }

    Vector2 medida = ui_medir_texto(fonte, vez, 16.0f);
    ui_desenhar_texto(fonte, vez,
                      (Vector2){LARGURA_TELA - 24.0f - medida.x, 16.0f},
                      16.0f, RAYWHITE);

    char credibilidade[64];
    snprintf(credibilidade, sizeof credibilidade, "Credibilidade: %d",
             jogo->credibilidade);
    medida = ui_medir_texto(fonte, credibilidade, 15.0f);
    ui_desenhar_texto(fonte, credibilidade,
                      (Vector2){LARGURA_TELA - 24.0f - medida.x, 44.0f},
                      15.0f, COR_TEXTO_SUAVE);
}

static void desenhar_prompt(const TelaDebate *tela,
                            const UIFonte *fonte) {
    int id = alvo_indicado(tela);
    bool tem_turno = suspeito_do_turno(tela->jogo) != NULL;

    const char *texto = TEXTO_SELECIONE;
    Color cor = COR_DESTAQUE;

    if (debate_encerrado(tela->jogo)) {
        texto = TEXTO_RODA_ENCERRADA;
        cor = COR_DESTAQUE;
    } else if (!tem_turno) {
        texto = TEXTO_SEM_TURNOS;
        cor = COR_TEXTO_SUAVE;
    }

    Vector2 medida = ui_medir_texto(fonte, texto, 18.0f);
    Rectangle faixa = {
        (LARGURA_TELA - medida.x) / 2.0f - 16.0f,
        434.0f,
        medida.x + 32.0f,
        28.0f
    };
    DrawRectangleRec(faixa, (Color){10, 10, 16, 210});
    ui_desenhar_texto_centralizado(fonte, texto, faixa, 18.0f, cor);

    if (tem_turno && id >= 0) {
        const Suspeito *alvo = buscar_suspeito(tela->jogo, id);
        if (alvo != NULL) {
            char rotulo[128];
            snprintf(rotulo, sizeof rotulo, "Alvo: %s (%s)",
                     (alvo->nome != NULL) ? alvo->nome : "?",
                     (alvo->cargo != NULL) ? alvo->cargo : "?");
            Vector2 alvo_medida = ui_medir_texto(fonte, rotulo, 15.0f);
            ui_desenhar_texto(
                fonte, rotulo,
                (Vector2){(LARGURA_TELA - alvo_medida.x) / 2.0f, 412.0f},
                15.0f, (Color){190, 205, 235, 255});
        }
    }
}

static void desenhar_periodo(const TelaDebate *tela,
                             const UIFonte *fonte) {
    Rectangle painel = {276.0f, 392.0f, 408.0f, 84.0f};
    DrawRectangleRec(painel, COR_PAINEL);
    DrawRectangleLinesEx(painel, 2.0f, COR_BORDA);

    Rectangle titulo = {painel.x, painel.y + 5.0f, painel.width, 22.0f};
    ui_desenhar_texto_centralizado(fonte, "Em qual período interrogar?",
                                   titulo, 16.0f, RAYWHITE);

    for (int i = 0; i < TOTAL_PERIODOS; i++) {
        ui_desenhar_botao(fonte,
                          (BotaoUI *)&tela->botoes_periodo[i],
                          tela->botao_focado == i);
    }
}

static void desenhar_depoimento(const TelaDebate *tela,
                                const UIFonte *fonte) {
    if (tela->jogo->caderno == NULL || tela->id_depoimento <= 0) {
        return;
    }

    Depoimento *depoimento = buscar_depoimento_por_id(
        tela->jogo->caderno, tela->id_depoimento);
    if (depoimento == NULL) {
        return;
    }

    Rectangle painel = {96.0f, 88.0f, 768.0f, 376.0f};
    DrawRectangleRec(painel, (Color){16, 16, 26, 242});
    DrawRectangleLinesEx(painel, 2.0f, COR_BORDA);

    char titulo[192];
    snprintf(titulo, sizeof titulo, "Depoimento de %s",
             nome_suspeito(tela->jogo, depoimento->id_falante));
    ui_desenhar_texto(fonte, titulo, (Vector2){painel.x + 24.0f,
                                               painel.y + 18.0f},
                      24.0f, RAYWHITE);

    char subtitulo[192];
    snprintf(subtitulo, sizeof subtitulo,
             "[Dia %d, %s] sobre %s", depoimento->dia,
             nome_periodo(depoimento->periodo),
             nome_suspeito(tela->jogo, depoimento->id_assunto));
    ui_desenhar_texto(fonte, subtitulo,
                      (Vector2){painel.x + 24.0f, painel.y + 52.0f},
                      16.0f, COR_TEXTO_SUAVE);

    DrawRectangle((int)painel.x + 24, (int)painel.y + 80,
                  (int)painel.width - 48, 1,
                  (Color){80, 80, 105, 255});

    Rectangle area_texto = {painel.x + 24.0f, painel.y + 96.0f,
                            painel.width - 48.0f, 110.0f};
    BeginScissorMode((int)area_texto.x, (int)area_texto.y,
                     (int)area_texto.width, (int)area_texto.height);
    ui_desenhar_texto_quebrado(
        fonte,
        (depoimento->texto != NULL) ? depoimento->texto
                                    : "Depoimento sem texto.",
        area_texto, 21.0f, 4.0f, RAYWHITE);
    EndScissorMode();

    ui_desenhar_texto(fonte, "Reações:",
                      (Vector2){painel.x + 24.0f, painel.y + 218.0f},
                      17.0f, COR_TEXTO_SUAVE);

    int reacoes = 0;
    for (NoListaDupla *no = tela->jogo->caderno->reacoes.inicio;
         no != NULL; no = no->prox) {
        const Reacao *reacao = (const Reacao *)no->dado;
        if (reacao == NULL ||
            reacao->id_depoimento != tela->id_depoimento) {
            continue;
        }

        int coluna = reacoes % 2;
        int linha = reacoes / 2;
        float x = painel.x + 40.0f + (float)coluna * 352.0f;
        float y = painel.y + 250.0f + (float)linha * 32.0f;

        ui_desenhar_icone(icone_da_reacao(reacao->tipo_reacao),
                          (Vector2){x, y + 11.0f}, 22.0f,
                          cor_da_reacao(reacao->tipo_reacao));

        const Suspeito *quem = NULL;
        for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
            const Suspeito *suspeito = buscar_suspeito(tela->jogo, id);
            if (suspeito != NULL && reacao->quem_reagiu != NULL &&
                strcmp(suspeito->nome, reacao->quem_reagiu) == 0) {
                quem = suspeito;
                break;
            }
        }

        char rotulo[160];
        snprintf(rotulo, sizeof rotulo, "%s — %s",
                 (reacao->quem_reagiu != NULL) ? reacao->quem_reagiu : "?",
                 (quem != NULL && quem->cargo != NULL) ? quem->cargo : "?");
        ui_desenhar_texto(fonte, rotulo, (Vector2){x + 22.0f, y + 2.0f},
                          16.0f, cor_da_reacao(reacao->tipo_reacao));
        reacoes++;
    }

    if (reacoes == 0) {
        ui_desenhar_texto(fonte, "Ninguém reagiu.",
                          (Vector2){painel.x + 40.0f, painel.y + 250.0f},
                          16.0f, COR_TEXTO_SUAVE);
    }

    if (tela->tempo_alerta > 0.0f) {
        ui_desenhar_icone(UI_ICONE_ALERTA,
                          (Vector2){painel.x + 34.0f,
                                    painel.y + 354.0f},
                          22.0f, COR_ALERTA);
        ui_desenhar_texto(fonte, TEXTO_NOVA_CONTRADICAO,
                          (Vector2){painel.x + 52.0f,
                                    painel.y + 344.0f},
                          17.0f, COR_ALERTA);
    }
}

static void desenhar_barra_acoes(const TelaDebate *tela,
                                 const UIFonte *fonte) {
    DrawRectangle(0, (int)(ALTURA_TELA - ALTURA_BARRA_ACOES),
                  (int)LARGURA_TELA, (int)ALTURA_BARRA_ACOES,
                  (Color){10, 10, 16, 235});
    DrawRectangle(0, (int)(ALTURA_TELA - ALTURA_BARRA_ACOES),
                  (int)LARGURA_TELA, 1, (Color){70, 70, 95, 220});

    ui_desenhar_botao(fonte, (BotaoUI *)&tela->botao_caderno, false);
    ui_desenhar_botao(fonte, (BotaoUI *)&tela->botao_interrogar, false);

    // Com a roda encerrada, "Encerrar debate" é a ação sugerida.
    ui_desenhar_botao(fonte, (BotaoUI *)&tela->botao_encerrar,
                      debate_encerrado(tela->jogo));
}

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------
void tela_debate_iniciar(TelaDebate *tela, Jogo *jogo) {
    if (tela == NULL) {
        return;
    }

    memset(tela, 0, sizeof *tela);
    tela->jogo = jogo;
    tela->etapa = DEBATE_ESCOLHENDO_ALVO;
    tela->id_alvo = primeiro_vivo(jogo);
    tela->id_depoimento = -1;
    tela->contradicoes_antes =
        (jogo != NULL && jogo->caderno != NULL)
            ? jogo->caderno->contradicoes.tamanho
            : 0;
    tela->tempo_alerta = 0.0f;
    tela->botao_focado = -1;
    tela->interrogar_pressionado = false;

    tela->botao_interrogar = (BotaoUI){
        .area = {380.0f, 486.0f, 200.0f, 40.0f},
        .rotulo = "Interrogar",
        .habilitado = true,
    };
    tela->botao_caderno = (BotaoUI){
        .area = {24.0f, 486.0f, 200.0f, 40.0f},
        .rotulo = "Abrir caderno",
        .habilitado = true,
    };
    tela->botao_encerrar = (BotaoUI){
        .area = {736.0f, 486.0f, 200.0f, 40.0f},
        .rotulo = "Encerrar debate",
        .habilitado = true,
    };

    // Assets primeiro: a área clicável de cada suspeito usa a proporção do
    // sprite real quando ele está disponível.
    tela->fundo = carregar_textura(DEBATE_FUNDO);
    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        tela->retratos[id] = carregar_textura(RETRATOS[id]);
    }

    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        tela->botoes_alvo[id] = (BotaoUI){
            .area = area_suspeito(tela, id),
            .rotulo = NULL,
            .habilitado = true,
        };
    }

    for (int i = 0; i < TOTAL_PERIODOS; i++) {
        tela->botoes_periodo[i] = (BotaoUI){
            .area = {300.0f + (float)i * 124.0f, 424.0f, 112.0f, 38.0f},
            .rotulo = ROTULOS_PERIODO[i],
            .habilitado = true,
        };
    }
}

static EntradaBotao ler_entrada(void) {
    EntradaBotao entrada = {
        .mouse = GetMousePosition(),
        .apertou = IsMouseButtonPressed(MOUSE_BUTTON_LEFT),
        .pressionado = IsMouseButtonDown(MOUSE_BUTTON_LEFT),
        .soltou = IsMouseButtonReleased(MOUSE_BUTTON_LEFT),
        .confirmar = IsKeyPressed(KEY_ENTER) ||
                     IsKeyPressed(KEY_KP_ENTER) ||
                     IsKeyPressed(KEY_SPACE),
    };
    return entrada;
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

    // Navegação por teclado entre os alvos vivos.
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

        if (entrada.confirmar && suspeito_vivo(jogo, tela->id_alvo)) {
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

    // Sem mais turnos: Enter encerra o debate direto, como o botão.
    if (debate_encerrado(jogo) && entrada.confirmar &&
        tela->etapa == DEBATE_ESCOLHENDO_ALVO) {
        return TELA_DEBATE_ENCERRAR;
    }

    // Alvos: só são clicáveis quando é a vez de escolher.
    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        tela->botoes_alvo[id].habilitado =
            pode_interrogar &&
            tela->etapa == DEBATE_ESCOLHENDO_ALVO &&
            suspeito_vivo(jogo, id) != NULL;

        EstadoBotao estado = ui_atualizar_botao(
            &tela->botoes_alvo[id], false, &entrada);

        if (tela->etapa == DEBATE_ESCOLHENDO_ALVO &&
            pode_interrogar) {
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
                tela->contradicoes_antes = antes;
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

    // Botões de ação.
    tela->botao_interrogar.habilitado = true;
    if (tela->etapa == DEBATE_ESCOLHENDO_ALVO) {
        tela->botao_interrogar.rotulo = "Interrogar";
        tela->botao_interrogar.habilitado =
            pode_interrogar && suspeito_vivo(jogo, tela->id_alvo) != NULL;
    } else if (tela->etapa == DEBATE_ESCOLHENDO_PERIODO) {
        tela->botao_interrogar.rotulo = "Interrogar";
        tela->botao_interrogar.habilitado = false;
    } else {
        tela->botao_interrogar.rotulo = "Continuar";
        tela->botao_interrogar.habilitado = true;
    }

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
    if (ui_atualizar_botao(&tela->botao_encerrar, false, &entrada) ==
            BOTAO_ACIONADO ||
        IsKeyPressed(KEY_E)) {
        return TELA_DEBATE_ENCERRAR;
    }

    return TELA_DEBATE_CONTINUAR;
}

void tela_debate_desenhar(const TelaDebate *tela, const UIFonte *fonte) {
    if (tela == NULL || tela->jogo == NULL) {
        return;
    }

    desenhar_fundo(tela);

    // Corpos primeiro; a mesa cobre a parte de baixo de quem está atrás dela.
    static const int ORDEM_DESENHO[TOTAL_SUSPEITOS] = {2, 3, 1, 4, 0, 5};
    for (int i = 0; i < TOTAL_SUSPEITOS; i++) {
        desenhar_corpo_suspeito(tela, ORDEM_DESENHO[i]);
    }

    desenhar_mesa();

    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        desenhar_situacao_suspeito(tela, id);
    }

    int indicado = alvo_indicado(tela);
    if (indicado >= 0) {
        desenhar_indicador(tela, fonte, indicado);
    }

    desenhar_cabecalho(tela, fonte);

    if (tela->etapa == DEBATE_MOSTRANDO_DEPOIMENTO) {
        desenhar_depoimento(tela, fonte);
    } else if (tela->etapa == DEBATE_ESCOLHENDO_PERIODO) {
        desenhar_periodo(tela, fonte);
    } else {
        desenhar_prompt(tela, fonte);
    }

    desenhar_barra_acoes(tela, fonte);
}
