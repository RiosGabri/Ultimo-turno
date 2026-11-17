#include "ui.h"

#include <stdio.h>
#include <stdlib.h>

#include "paleta.h"

#define UI_MAX_CODEPOINTS 512

// ---------------------------------------------------------------------------
// Fonte
// ---------------------------------------------------------------------------
const int *ui_codepoints_portugues(int *quantidade) {
    static int codepoints[UI_MAX_CODEPOINTS];
    static int total = 0;

    if (total == 0) {
        int n = 0;

        // ASCII imprimível: letras sem acento, números e pontuação básica.
        for (int c = 0x20; c <= 0x7E && n < UI_MAX_CODEPOINTS; c++) {
            codepoints[n++] = c;
        }

        // Latin-1 Supplement e Latin Extended-A: á à â ã ä, ç, é ê, í, ó ô õ,
        // ú ü, ñ etc., em minúsculas e maiúsculas.
        for (int c = 0xA0; c <= 0x17F && n < UI_MAX_CODEPOINTS; c++) {
            codepoints[n++] = c;
        }

        // Pontuação tipográfica usada nos textos do jogo.
        static const int tipografia[] = {
            0x2013, // –
            0x2014, // —
            0x2018, // ‘
            0x2019, // ’
            0x201A, // ‚
            0x201C, // “
            0x201D, // ”
            0x201E, // „
            0x2022, // •
            0x2026, // …
            0x2039, // ‹
            0x203A, // ›
            0x20AC  // €
        };
        int total_tipografia = (int)(sizeof(tipografia) / sizeof(tipografia[0]));
        for (int i = 0; i < total_tipografia && n < UI_MAX_CODEPOINTS; i++) {
            codepoints[n++] = tipografia[i];
        }

        total = n;
    }

    if (quantidade != NULL) {
        *quantidade = total;
    }
    return codepoints;
}

const char *ui_caminho_fonte(const char *caminho) {
    static char encontrado[1024];
    const char *alvo = (caminho != NULL) ? caminho : UI_CAMINHO_FONTE;

    if (FileExists(alvo)) {
        return alvo;
    }

    // Pasta do executável (o CMake copia assets/ para lá).
    const char *pasta_aplicacao = GetApplicationDirectory();
    if (pasta_aplicacao != NULL) {
        snprintf(encontrado, sizeof(encontrado), "%s%s", pasta_aplicacao,
                 alvo);
        if (FileExists(encontrado)) {
            return encontrado;
        }
    }

    // Diretório atual e um ou dois níveis acima: permite rodar a partir da
    // raiz do repositório, de build/ ou de build/Release/.
    snprintf(encontrado, sizeof(encontrado), "../%s", alvo);
    if (FileExists(encontrado)) {
        return encontrado;
    }

    snprintf(encontrado, sizeof(encontrado), "../../%s", alvo);
    if (FileExists(encontrado)) {
        return encontrado;
    }

    return NULL;
}

UIFonte ui_carregar_fonte(const char *caminho, int tamanho_base) {
    UIFonte resultado = {0};
    resultado.fonte = GetFontDefault();
    resultado.tamanho_base =
        (tamanho_base > 0) ? tamanho_base : UI_FONTE_TAMANHO_BASE;
    resultado.carregada = false;

    const char *encontrado = ui_caminho_fonte(caminho);
    if (encontrado == NULL) {
        TraceLog(LOG_WARNING,
                 "UI: fonte '%s' nao encontrada; usando a fonte padrao da "
                 "raylib (sem acentos).",
                 (caminho != NULL) ? caminho : UI_CAMINHO_FONTE);
        return resultado;
    }

    int quantidade = 0;
    const int *codepoints = ui_codepoints_portugues(&quantidade);
    Font carregada = LoadFontEx(encontrado, resultado.tamanho_base, codepoints,
                                quantidade);

    if (!IsFontValid(carregada)) {
        TraceLog(LOG_WARNING,
                 "UI: falha ao carregar '%s'; usando a fonte padrao da raylib.",
                 encontrado);
        return resultado;
    }

    // A fonte é rasterizada grande e desenhada menor na maior parte do tempo;
    // o filtro bilinear suaviza a redução.
    SetTextureFilter(carregada.texture, TEXTURE_FILTER_BILINEAR);

    resultado.fonte = carregada;
    resultado.carregada = true;
    TraceLog(LOG_INFO, "UI: fonte '%s' carregada com %d glifos (base %d).",
             encontrado, carregada.glyphCount, resultado.tamanho_base);
    return resultado;
}

void ui_descarregar_fonte(UIFonte *fonte) {
    if (fonte == NULL) {
        return;
    }

    if (fonte->carregada) {
        UnloadFont(fonte->fonte);
    }

    fonte->fonte = GetFontDefault();
    fonte->tamanho_base = UI_FONTE_TAMANHO_BASE;
    fonte->carregada = false;
}

bool ui_fonte_suporta_acentos(const UIFonte *fonte) {
    static const int acentos[] = {
        0x00E1, // á
        0x00E7, // ç
        0x00E3, // ã
        0x00E9, // é
        0x00F4  // ô
    };

    if (fonte == NULL || !IsFontValid(fonte->fonte)) {
        return false;
    }

    for (int i = 0; i < (int)(sizeof(acentos) / sizeof(acentos[0])); i++) {
        int indice = GetGlyphIndex(fonte->fonte, acentos[i]);
        if (indice < 0 || indice >= fonte->fonte.glyphCount ||
            fonte->fonte.glyphs[indice].value != acentos[i]) {
            return false;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// Texto
// ---------------------------------------------------------------------------
static Font fonte_para_desenho(const UIFonte *fonte) {
    if (fonte != NULL && IsFontValid(fonte->fonte)) {
        return fonte->fonte;
    }
    return GetFontDefault();
}

static float tamanho_valido(float tamanho) {
    return (tamanho > 0.0f) ? tamanho : 10.0f;
}

// Mantém o espaçamento entre letras proporcional ao tamanho pedido, para o
// texto medido ser igual ao texto desenhado.
static float espacamento_texto(float tamanho) {
    return tamanho / 32.0f;
}

Vector2 ui_medir_texto(const UIFonte *fonte, const char *texto, float tamanho) {
    if (texto == NULL || texto[0] == '\0') {
        return (Vector2){0.0f, 0.0f};
    }

    float t = tamanho_valido(tamanho);
    return MeasureTextEx(fonte_para_desenho(fonte), texto, t,
                         espacamento_texto(t));
}

void ui_desenhar_texto(const UIFonte *fonte, const char *texto, Vector2 posicao,
                       float tamanho, Color cor) {
    if (texto == NULL || texto[0] == '\0') {
        return;
    }

    float t = tamanho_valido(tamanho);
    DrawTextEx(fonte_para_desenho(fonte), texto, posicao, t,
               espacamento_texto(t), cor);
}

void ui_desenhar_texto_centralizado(const UIFonte *fonte, const char *texto,
                                    Rectangle area, float tamanho, Color cor) {
    Vector2 medida = ui_medir_texto(fonte, texto, tamanho);
    Vector2 posicao = {
        area.x + (area.width - medida.x) / 2.0f,
        area.y + (area.height - medida.y) / 2.0f
    };
    ui_desenhar_texto(fonte, texto, posicao, tamanho, cor);
}

void ui_desenhar_icone(UIIcone icone, Vector2 centro, float tamanho, Color cor) {
    if (tamanho <= 0.0f ||
        (icone != UI_ICONE_CONCORDA && icone != UI_ICONE_DISCORDA &&
         icone != UI_ICONE_NEUTRO && icone != UI_ICONE_ALERTA)) {
        return;
    }

    float raio = tamanho * 0.43f;
    float espessura = tamanho * 0.075f;
    if (espessura < 1.0f) {
        espessura = 1.0f;
    }

    if (icone == UI_ICONE_ALERTA) {
        Vector2 topo = {centro.x, centro.y - raio};
        Vector2 base_esquerda = {centro.x - raio, centro.y + raio * 0.72f};
        Vector2 base_direita = {centro.x + raio, centro.y + raio * 0.72f};
        DrawLineEx(topo, base_esquerda, espessura, cor);
        DrawLineEx(base_esquerda, base_direita, espessura, cor);
        DrawLineEx(base_direita, topo, espessura, cor);
        DrawLineEx((Vector2){centro.x, centro.y - raio * 0.38f},
                   (Vector2){centro.x, centro.y + raio * 0.24f}, espessura,
                   cor);
        DrawCircleV((Vector2){centro.x, centro.y + raio * 0.52f},
                    espessura * 0.65f, cor);
        return;
    }

    DrawCircleLines((int)centro.x, (int)centro.y, raio, cor);

    switch (icone) {
        case UI_ICONE_CONCORDA:
            DrawLineEx((Vector2){centro.x - raio * 0.52f,
                                 centro.y + raio * 0.02f},
                       (Vector2){centro.x - raio * 0.12f,
                                 centro.y + raio * 0.42f},
                       espessura, cor);
            DrawLineEx((Vector2){centro.x - raio * 0.12f,
                                 centro.y + raio * 0.42f},
                       (Vector2){centro.x + raio * 0.56f,
                                 centro.y - raio * 0.40f},
                       espessura, cor);
            break;
        case UI_ICONE_DISCORDA:
            DrawLineEx((Vector2){centro.x - raio * 0.38f,
                                 centro.y - raio * 0.38f},
                       (Vector2){centro.x + raio * 0.38f,
                                 centro.y + raio * 0.38f},
                       espessura, cor);
            DrawLineEx((Vector2){centro.x + raio * 0.38f,
                                 centro.y - raio * 0.38f},
                       (Vector2){centro.x - raio * 0.38f,
                                 centro.y + raio * 0.38f},
                       espessura, cor);
            break;
        case UI_ICONE_NEUTRO:
            DrawCircleV((Vector2){centro.x - raio * 0.32f,
                                  centro.y - raio * 0.16f},
                        espessura * 0.55f, cor);
            DrawCircleV((Vector2){centro.x + raio * 0.32f,
                                  centro.y - raio * 0.16f},
                        espessura * 0.55f, cor);
            DrawLineEx((Vector2){centro.x - raio * 0.38f,
                                 centro.y + raio * 0.28f},
                       (Vector2){centro.x + raio * 0.38f,
                                 centro.y + raio * 0.28f},
                       espessura, cor);
            break;
        default:
            return;
    }
}

float ui_desenhar_painel_texto(const UIFonte *fonte, Rectangle area,
                               const char *texto, float tamanho_texto,
                               Color fundo, Color borda, Color cor_texto) {
    const float padding = 14.0f;
    DrawRectangleRec(area, fundo);
    DrawRectangleLinesEx(area, 2.0f, borda);

    float largura_interna = area.width - 2.0f * padding;
    float altura_interna = area.height - 2.0f * padding;
    if (texto == NULL || largura_interna <= 0.0f || altura_interna <= 0.0f) {
        return 0.0f;
    }

    LayoutTexto layout = ui_quebrar_texto(fonte, texto, largura_interna,
                                          tamanho_texto, 0.0f);
    BeginScissorMode((int)(area.x + padding), (int)(area.y + padding),
                     (int)largura_interna, (int)altura_interna);
    ui_desenhar_layout_texto(fonte, &layout,
                             (Vector2){area.x + padding, area.y + padding},
                             cor_texto);
    EndScissorMode();

    float altura = layout.altura;
    ui_liberar_layout_texto(&layout);
    return altura;
}

// Largura de avanço de um único codepoint, já escalada para o tamanho pedido.
static float avanco_codepoint(const Font *fonte, int codepoint, float escala) {
    int indice = GetGlyphIndex(*fonte, codepoint);

    if (fonte->glyphs[indice].advanceX > 0) {
        return (float)fonte->glyphs[indice].advanceX * escala;
    }

    return ((float)fonte->recs[indice].width +
            (float)fonte->glyphs[indice].offsetX) * escala;
}

// Mede um trecho sem precisar de string terminada em '\0'. Usa os mesmos
// avanços e espaçamento do DrawTextEx, mas só com os dados da fonte (sem GPU).
static float largura_trecho(const Font *fonte, const char *inicio,
                            const char *fim, float tamanho,
                            float espacamento) {
    float escala = tamanho / (float)fonte->baseSize;
    float largura = 0.0f;
    int quantidade = 0;
    const char *p = inicio;

    while (p < fim) {
        int bytes = 0;
        int codepoint = GetCodepointNext(p, &bytes);
        if (bytes <= 0 || p + bytes > fim) {
            break;
        }

        if (quantidade > 0) {
            largura += espacamento;
        }
        largura += avanco_codepoint(fonte, codepoint, escala);
        quantidade++;
        p += bytes;
    }

    return largura;
}

static bool layout_adicionar(LayoutTexto *layout, int *capacidade,
                             const char *inicio, int comprimento,
                             float largura) {
    if (layout->quantidade == *capacidade) {
        int nova_capacidade = (*capacidade == 0) ? 8 : *capacidade * 2;
        LinhaTexto *novas = realloc(
            layout->linhas, (size_t)nova_capacidade * sizeof(*novas));
        if (novas == NULL) {
            return false;
        }
        layout->linhas = novas;
        *capacidade = nova_capacidade;
    }

    layout->linhas[layout->quantidade].inicio = inicio;
    layout->linhas[layout->quantidade].comprimento = comprimento;
    layout->linhas[layout->quantidade].largura = largura;
    layout->quantidade++;
    return true;
}

LayoutTexto ui_quebrar_texto(const UIFonte *fonte, const char *texto,
                             float largura_maxima, float tamanho,
                             float espacamento_linhas) {
    LayoutTexto layout = {0};

    if (texto == NULL || texto[0] == '\0' || largura_maxima <= 0.0f) {
        return layout;
    }

    Font f = fonte_para_desenho(fonte);
    if (!IsFontValid(f) || f.baseSize <= 0) {
        return layout;
    }

    float t = tamanho_valido(tamanho);
    float espaco_letras = espacamento_texto(t);
    float espaco_linhas =
        (espacamento_linhas > 0.0f) ? espacamento_linhas : t * 0.25f;
    float escala = t / (float)f.baseSize;

    int capacidade = 0;
    bool ok = true;
    const char *inicio_linha = NULL;
    const char *fim_linha = NULL;
    float largura_linha = 0.0f;

    const char *p = texto;
    while (*p != '\0' && ok) {
        if (*p == '\n') {
            // Fecha a linha atual, inclusive vazia, para preservar parágrafos.
            int comprimento =
                (inicio_linha != NULL) ? (int)(p - inicio_linha) : 0;
            if (inicio_linha == NULL) {
                inicio_linha = p;
            }
            ok = layout_adicionar(&layout, &capacidade, inicio_linha,
                                  comprimento, largura_linha);

            p++;
            inicio_linha = NULL;
            fim_linha = NULL;
            largura_linha = 0.0f;
            continue;
        }

        if (*p == ' ' || *p == '\t' || *p == '\r') {
            p++;
            continue;
        }

        const char *inicio_palavra = p;
        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\r' &&
               *p != '\n') {
            p++;
        }
        const char *fim_palavra = p;

        float largura_palavra =
            largura_trecho(&f, inicio_palavra, fim_palavra, t, espaco_letras);

        if (inicio_linha != NULL) {
            // Mede a linha inteira com esta palavra para considerar os espaços
            // que ficaram entre as palavras.
            float candidata =
                largura_trecho(&f, inicio_linha, fim_palavra, t, espaco_letras);
            if (candidata <= largura_maxima) {
                fim_linha = fim_palavra;
                largura_linha = candidata;
                continue;
            }

            // Não cabe: fecha a linha atual e recomeça com a palavra.
            ok = layout_adicionar(&layout, &capacidade, inicio_linha,
                                  (int)(fim_linha - inicio_linha),
                                  largura_linha);
            inicio_linha = NULL;
            fim_linha = NULL;
            largura_linha = 0.0f;
        }

        if (!ok) {
            break;
        }

        if (largura_palavra <= largura_maxima) {
            inicio_linha = inicio_palavra;
            fim_linha = fim_palavra;
            largura_linha = largura_palavra;
            continue;
        }

        // Palavra maior que a largura disponível: parte por codepoint, sem
        // hifenizar, para o texto nunca estourar a área.
        const char *inicio_trecho = inicio_palavra;
        const char *p_trecho = inicio_palavra;
        float largura_atual = 0.0f;
        bool tem_codepoint = false;

        while (p_trecho < fim_palavra) {
            int bytes = 0;
            int codepoint = GetCodepointNext(p_trecho, &bytes);
            if (bytes <= 0 || p_trecho + bytes > fim_palavra) {
                break;
            }

            float avanco = avanco_codepoint(&f, codepoint, escala);
            if (tem_codepoint) {
                avanco += espaco_letras;
            }

            if (tem_codepoint && largura_atual + avanco > largura_maxima) {
                ok = layout_adicionar(&layout, &capacidade, inicio_trecho,
                                      (int)(p_trecho - inicio_trecho),
                                      largura_atual);
                if (!ok) {
                    break;
                }
                inicio_trecho = p_trecho;
                largura_atual = 0.0f;
                tem_codepoint = false;
                continue; // reavalia o codepoint na nova linha
            }

            largura_atual += avanco;
            tem_codepoint = true;
            p_trecho += bytes;
        }

        if (!ok) {
            break;
        }

        if (tem_codepoint) {
            inicio_linha = inicio_trecho;
            fim_linha = fim_palavra;
            largura_linha = largura_atual;
        }
    }

    // Fecha a última linha, se houver conteúdo pendente.
    if (ok && inicio_linha != NULL && fim_linha != NULL) {
        ok = layout_adicionar(&layout, &capacidade, inicio_linha,
                              (int)(fim_linha - inicio_linha), largura_linha);
    }

    // Texto só com espaços ainda ocupa uma linha vazia.
    if (ok && layout.quantidade == 0) {
        ok = layout_adicionar(&layout, &capacidade, texto, 0, 0.0f);
    }

    if (!ok) {
        free(layout.linhas);
        return (LayoutTexto){0};
    }

    layout.tamanho = t;
    layout.espacamento_linhas = espaco_linhas;
    if (layout.quantidade > 0) {
        layout.altura = (float)layout.quantidade * t +
                        (float)(layout.quantidade - 1) * espaco_linhas;
    }
    return layout;
}

void ui_liberar_layout_texto(LayoutTexto *layout) {
    if (layout == NULL) {
        return;
    }

    free(layout->linhas);
    *layout = (LayoutTexto){0};
}

float ui_desenhar_layout_texto(const UIFonte *fonte, const LayoutTexto *layout,
                               Vector2 posicao, Color cor) {
    if (layout == NULL || layout->linhas == NULL || layout->quantidade <= 0) {
        return 0.0f;
    }

    Font f = fonte_para_desenho(fonte);
    if (!IsFontValid(f) || f.baseSize <= 0) {
        return 0.0f;
    }

    float escala = layout->tamanho / (float)f.baseSize;
    float espaco_letras = espacamento_texto(layout->tamanho);
    float y = posicao.y;

    for (int i = 0; i < layout->quantidade; i++) {
        const LinhaTexto *linha = &layout->linhas[i];
        const char *p = linha->inicio;
        const char *fim = linha->inicio + linha->comprimento;
        float x = posicao.x;
        int quantidade = 0;

        while (p < fim) {
            int bytes = 0;
            int codepoint = GetCodepointNext(p, &bytes);
            if (bytes <= 0 || p + bytes > fim) {
                break;
            }

            if (quantidade > 0) {
                x += espaco_letras;
            }
            if (codepoint != ' ' && codepoint != '\t') {
                DrawTextCodepoint(f, codepoint, (Vector2){x, y},
                                  layout->tamanho, cor);
            }
            x += avanco_codepoint(&f, codepoint, escala);
            quantidade++;
            p += bytes;
        }

        y += layout->tamanho + layout->espacamento_linhas;
    }

    return layout->altura;
}

float ui_desenhar_texto_quebrado(const UIFonte *fonte, const char *texto,
                                 Rectangle area, float tamanho,
                                 float espacamento_linhas, Color cor) {
    LayoutTexto layout =
        ui_quebrar_texto(fonte, texto, area.width, tamanho, espacamento_linhas);
    float altura = ui_desenhar_layout_texto(fonte, &layout,
                                            (Vector2){area.x, area.y}, cor);
    ui_liberar_layout_texto(&layout);
    return altura;
}

float ui_medir_altura_texto_quebrado(const UIFonte *fonte, const char *texto,
                                     float largura_maxima, float tamanho,
                                     float espacamento_linhas) {
    LayoutTexto layout =
        ui_quebrar_texto(fonte, texto, largura_maxima, tamanho, espacamento_linhas);
    float altura = layout.altura;
    ui_liberar_layout_texto(&layout);
    return altura;
}

// ---------------------------------------------------------------------------
// Botões
// ---------------------------------------------------------------------------
EstadoBotao ui_atualizar_botao(BotaoUI *botao, bool focado,
                               const EntradaBotao *entrada) {
    if (botao == NULL) {
        return BOTAO_DESABILITADO;
    }

    if (!botao->habilitado) {
        botao->pressionado = false;
        return BOTAO_DESABILITADO;
    }

    bool dentro = false;

    if (entrada != NULL) {
        dentro = CheckCollisionPointRec(entrada->mouse, botao->area);

        if (entrada->apertou && dentro) {
            botao->pressionado = true;
        }

        if (entrada->soltou) {
            bool acionou = botao->pressionado && dentro;
            botao->pressionado = false;
            if (acionou) {
                return BOTAO_ACIONADO;
            }
        }

        if (focado && entrada->confirmar) {
            botao->pressionado = false;
            return BOTAO_ACIONADO;
        }

        if (botao->pressionado && entrada->pressionado && dentro) {
            return BOTAO_PRESSIONADO;
        }

        // Se o botão do mouse não está mais pressionado, não guarde um estado
        // antigo (evita acionar num clique que começou em outro lugar).
        if (!entrada->apertou && !entrada->pressionado) {
            botao->pressionado = false;
        }
    }

    if (focado || dentro) {
        return BOTAO_HOVER;
    }

    return BOTAO_NORMAL;
}

bool ui_desenhar_botao(const UIFonte *fonte, BotaoUI *botao, bool focado) {
    if (botao == NULL || botao->rotulo == NULL) {
        return false;
    }

    EntradaBotao entrada = {
        .mouse = GetMousePosition(),
        .apertou = IsMouseButtonPressed(MOUSE_BUTTON_LEFT),
        .pressionado = IsMouseButtonDown(MOUSE_BUTTON_LEFT),
        .soltou = IsMouseButtonReleased(MOUSE_BUTTON_LEFT),
        .confirmar = IsKeyPressed(KEY_ENTER) ||
                     IsKeyPressed(KEY_KP_ENTER) ||
                     IsKeyPressed(KEY_SPACE),
    };

    EstadoBotao estado = ui_atualizar_botao(botao, focado, &entrada);

    Color fundo = botao->primario ? COR_OXIDO : COR_FUMACA;
    Color borda = botao->primario ? COR_TIJOLO : COR_POEIRA;
    Color texto = COR_OSSO;

    switch (estado) {
        case BOTAO_DESABILITADO:
            fundo = COR_TINTA;
            borda = COR_FUMACA;
            texto = COR_POEIRA;
            break;

        case BOTAO_ACIONADO:
        case BOTAO_PRESSIONADO:
            fundo = botao->primario ? COR_SANGUE : COR_TINTA;
            borda = COR_TIJOLO;
            break;

        case BOTAO_HOVER:
            fundo = botao->primario ? COR_TIJOLO : COR_CARVAO;
            borda = COR_OSSO;
            break;

        case BOTAO_NORMAL:
        default:
            break;
    }

    DrawRectangleRec(botao->area, fundo);
    DrawRectangleLinesEx(botao->area, (estado == BOTAO_HOVER) ? 2.0f : 1.0f,
                         borda);

    float tamanho_texto = botao->area.height * 0.42f;
    if (tamanho_texto < 10.0f) {
        tamanho_texto = 10.0f;
    }

    ui_desenhar_texto_centralizado(fonte, botao->rotulo, botao->area,
                                   tamanho_texto, texto);

    return estado == BOTAO_ACIONADO;
}

int ui_navegar_botoes(const BotaoUI *botoes, int quantidade, int focado,
                      int passo) {
    if (botoes == NULL || quantidade <= 0) {
        return -1;
    }

    int direcao = (passo >= 0) ? 1 : -1;
    int atual = focado;

    if (atual < 0 || atual >= quantidade) {
        atual = (direcao > 0) ? -1 : quantidade;
    }

    for (int i = 0; i < quantidade; i++) {
        atual += direcao;
        if (atual < 0) {
            atual = quantidade - 1;
        } else if (atual >= quantidade) {
            atual = 0;
        }

        if (botoes[atual].habilitado) {
            return atual;
        }
    }

    return -1;
}
