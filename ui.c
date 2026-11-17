#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UI_MAX_CODEPOINTS 512

static const Color COR_FUNDO_BOTAO = {45, 45, 60, 255};
static const Color COR_FUNDO_BOTAO_DESTACADO = {80, 80, 110, 255};
static const Color COR_FUNDO_BOTAO_DESABILITADO = {35, 35, 42, 255};
static const Color COR_BORDA_BOTAO = {120, 120, 140, 255};
static const Color COR_BORDA_BOTAO_DESTACADA = {245, 245, 245, 255};
static const Color COR_TEXTO_BOTAO = {245, 245, 245, 255};
static const Color COR_TEXTO_BOTAO_DESABILITADO = {110, 110, 125, 255};

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

    if (caminho != NULL && FileExists(caminho)) {
        return caminho;
    }

    // Pasta do executável (o CMake copia assets/ para lá).
    const char *pasta_aplicacao = GetApplicationDirectory();
    if (pasta_aplicacao != NULL) {
        snprintf(encontrado, sizeof(encontrado), "%s%s", pasta_aplicacao,
                 UI_CAMINHO_FONTE);
        if (FileExists(encontrado)) {
            return encontrado;
        }
    }

    // Diretório atual e um ou dois níveis acima: permite rodar a partir da
    // raiz do repositório, de build/ ou de build/Release/.
    if (FileExists(UI_CAMINHO_FONTE)) {
        return UI_CAMINHO_FONTE;
    }

    snprintf(encontrado, sizeof(encontrado), "../%s", UI_CAMINHO_FONTE);
    if (FileExists(encontrado)) {
        return encontrado;
    }

    snprintf(encontrado, sizeof(encontrado), "../../%s", UI_CAMINHO_FONTE);
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

float ui_desenhar_texto_quebrado(const UIFonte *fonte, const char *texto,
                                 Rectangle area, float tamanho,
                                 float espacamento_linhas, Color cor) {
    if (texto == NULL || texto[0] == '\0' || area.width <= 0.0f) {
        return 0.0f;
    }

    size_t comprimento = strlen(texto);
    char *copia = malloc(comprimento + 1);
    if (copia == NULL) {
        return 0.0f;
    }
    memcpy(copia, texto, comprimento + 1);

    float t = tamanho_valido(tamanho);
    float espaco_letras = espacamento_texto(t);
    float altura_linha =
        t + ((espacamento_linhas > 0.0f) ? espacamento_linhas : t * 0.25f);
    float largura_espaco = MeasureTextEx(fonte_para_desenho(fonte), " ", t,
                                         espaco_letras).x;
    float x = area.x;
    float y = area.y;

    char *p = copia;
    while (*p != '\0') {
        if (*p == '\n') {
            x = area.x;
            y += altura_linha;
            p++;
            continue;
        }

        if (*p == ' ' || *p == '\t') {
            p++;
            continue;
        }

        // Isola a palavra para medir e desenhar sem copiar de novo.
        char *inicio = p;
        while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n') {
            p++;
        }
        char separador = *p;
        *p = '\0';

        float largura_palavra = MeasureTextEx(
            fonte_para_desenho(fonte), inicio, t, espaco_letras).x;

        if (x > area.x && x + largura_palavra > area.x + area.width) {
            x = area.x;
            y += altura_linha;
        }

        ui_desenhar_texto(fonte, inicio, (Vector2){x, y}, t, cor);
        x += largura_palavra;

        if (separador == '\0') {
            break;
        }

        if (separador == '\n') {
            x = area.x;
            y += altura_linha;
        } else {
            x += largura_espaco;
        }
        p++;
    }

    free(copia);
    return (y + t) - area.y;
}

// ---------------------------------------------------------------------------
// Botões
// ---------------------------------------------------------------------------
bool ui_desenhar_botao(const UIFonte *fonte, const BotaoUI *botao,
                       bool destacado) {
    if (botao == NULL || botao->rotulo == NULL) {
        return false;
    }

    Vector2 mouse = GetMousePosition();
    bool sob_mouse = CheckCollisionPointRec(mouse, botao->area);
    bool ativo = botao->habilitado && (destacado || sob_mouse);
    bool acionado = false;

    if (botao->habilitado) {
        if (sob_mouse && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            acionado = true;
        }
        if (destacado && (IsKeyPressed(KEY_ENTER) ||
                          IsKeyPressed(KEY_KP_ENTER) ||
                          IsKeyPressed(KEY_SPACE))) {
            acionado = true;
        }
    }

    Color fundo = COR_FUNDO_BOTAO;
    Color borda = COR_BORDA_BOTAO;
    Color texto = COR_TEXTO_BOTAO;

    if (!botao->habilitado) {
        fundo = COR_FUNDO_BOTAO_DESABILITADO;
        texto = COR_TEXTO_BOTAO_DESABILITADO;
    } else if (ativo) {
        fundo = COR_FUNDO_BOTAO_DESTACADO;
        borda = COR_BORDA_BOTAO_DESTACADA;
    }

    DrawRectangleRec(botao->area, fundo);
    DrawRectangleLinesEx(botao->area, 2.0f, borda);

    float tamanho_texto = botao->area.height * 0.42f;
    if (tamanho_texto < 10.0f) {
        tamanho_texto = 10.0f;
    }

    ui_desenhar_texto_centralizado(fonte, botao->rotulo, botao->area,
                                   tamanho_texto, texto);

    return acionado;
}
