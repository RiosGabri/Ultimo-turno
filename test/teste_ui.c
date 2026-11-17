#include "ui.h"

#include <stdio.h>
#include <string.h>

static int falhas = 0;

static void verificar(int condicao, const char *mensagem) {
    if (!condicao) {
        printf("FALHOU: %s\n", mensagem);
        falhas++;
    }
}

static void testar_codepoints_do_portugues(void) {
    static const int acentos[] = {0x00E1, 0x00E7, 0x00E3, 0x00E9, 0x00F4};
    static const char *nomes[] = {"á", "ç", "ã", "é", "ô"};
    int total_acentos = (int)(sizeof(acentos) / sizeof(acentos[0]));

    int quantidade = 0;
    const int *codepoints = ui_codepoints_portugues(&quantidade);

    verificar(codepoints != NULL, "a lista de codepoints não pode ser NULL");
    verificar(quantidade > 0, "a lista de codepoints não pode ser vazia");

    char mensagem[160];
    for (int i = 0; i < total_acentos; i++) {
        int encontrado = 0;
        for (int j = 0; j < quantidade; j++) {
            if (codepoints[j] == acentos[i]) {
                encontrado = 1;
                break;
            }
        }

        snprintf(mensagem, sizeof(mensagem),
                 "o codepoint de '%s' (U+%04X) deve estar na lista",
                 nomes[i], acentos[i]);
        verificar(encontrado, mensagem);
    }

    int ordenada = 1;
    for (int i = 1; i < quantidade; i++) {
        if (codepoints[i] <= codepoints[i - 1]) {
            ordenada = 0;
            break;
        }
    }
    verificar(ordenada,
              "os codepoints devem ser únicos e estar em ordem crescente");
}

static void testar_arquivo_da_fonte(void) {
    const char *caminho = ui_caminho_fonte(NULL);

    verificar(caminho != NULL, "a fonte .ttf deve existir em assets/fontes/");
    if (caminho != NULL) {
        printf("Fonte encontrada: %s\n", caminho);
    }
}

static void testar_glifos_acentuados(void) {
    static const int acentos[] = {0x00E1, 0x00E7, 0x00E3, 0x00E9, 0x00F4};
    static const char *nomes[] = {"á", "ç", "ã", "é", "ô"};
    int total_acentos = (int)(sizeof(acentos) / sizeof(acentos[0]));

    const char *caminho = ui_caminho_fonte(NULL);
    if (caminho == NULL) {
        return; // o teste anterior já registrou a falha
    }

    int tamanho_arquivo = 0;
    unsigned char *dados = LoadFileData(caminho, &tamanho_arquivo);
    verificar(dados != NULL && tamanho_arquivo > 0,
              "o arquivo da fonte deve poder ser lido");
    if (dados == NULL) {
        return;
    }

    int quantidade_codepoints = 0;
    const int *codepoints = ui_codepoints_portugues(&quantidade_codepoints);

    int quantidade_glifos = 0;
    GlyphInfo *glifos =
        LoadFontData(dados, tamanho_arquivo, UI_FONTE_TAMANHO_BASE, codepoints,
                     quantidade_codepoints, FONT_DEFAULT, &quantidade_glifos);

    verificar(glifos != NULL, "LoadFontData deve rasterizar o .ttf");
    verificar(quantidade_glifos > 0, "o .ttf deve produzir glifos");

    if (glifos != NULL) {
        char mensagem[160];
        for (int i = 0; i < total_acentos; i++) {
            int encontrado = 0;
            for (int j = 0; j < quantidade_glifos; j++) {
                if (glifos[j].value == acentos[i]) {
                    encontrado = 1;
                    break;
                }
            }

            snprintf(mensagem, sizeof(mensagem),
                     "o .ttf deve ter o glifo de '%s' (U+%04X)", nomes[i],
                     acentos[i]);
            verificar(encontrado, mensagem);
        }

        // Monta uma fonte só com os glifos em RAM (sem textura) para exercitar
        // a mesma checagem usada em ui_carregar_fonte.
        Rectangle recs[1] = {{0}};
        UIFonte fonte = {
            .fonte = {
                .baseSize = UI_FONTE_TAMANHO_BASE,
                .glyphCount = quantidade_glifos,
                .glyphPadding = 0,
                .texture = {0},
                .recs = recs,
                .glyphs = glifos,
            },
            .tamanho_base = UI_FONTE_TAMANHO_BASE,
            .carregada = true,
        };

        verificar(ui_fonte_suporta_acentos(&fonte),
                  "ui_fonte_suporta_acentos deve aceitar \"á ç ã é ô\"");

        UnloadFontData(glifos, quantidade_glifos);
    }

    UnloadFileData(dados);
}

static void testar_fonte_sem_acentos(void) {
    GlyphInfo glifo_fallback = {0};
    glifo_fallback.value = '?';

    Rectangle recs[1] = {{0}};
    UIFonte fonte = {
        .fonte = {
            .baseSize = 10,
            .glyphCount = 1,
            .glyphPadding = 0,
            .texture = {0},
            .recs = recs,
            .glyphs = &glifo_fallback,
        },
        .tamanho_base = 10,
        .carregada = false,
    };

    verificar(!ui_fonte_suporta_acentos(&fonte),
              "uma fonte só com '?' não deve passar na checagem de acentos");
}

// ---------------------------------------------------------------------------
// Quebra de linha (fonte sintética: cada caractere avança 10 pixels)
// ---------------------------------------------------------------------------
#define FONTE_TESTE_GLIFOS 128
#define FONTE_TESTE_BASE 10
#define FONTE_TESTE_AVANCO 10

static GlyphInfo glifos_teste[FONTE_TESTE_GLIFOS];
static Rectangle recs_teste[FONTE_TESTE_GLIFOS];

static UIFonte criar_fonte_teste(void) {
    for (int i = 0; i < FONTE_TESTE_GLIFOS; i++) {
        glifos_teste[i] = (GlyphInfo){0};
        glifos_teste[i].value = i;
        glifos_teste[i].advanceX = FONTE_TESTE_AVANCO;
        recs_teste[i] =
            (Rectangle){0, 0, FONTE_TESTE_AVANCO, FONTE_TESTE_BASE};
    }

    UIFonte fonte = {
        .fonte = {
            .baseSize = FONTE_TESTE_BASE,
            .glyphCount = FONTE_TESTE_GLIFOS,
            .glyphPadding = 0,
            .texture = {0},
            .recs = recs_teste,
            .glyphs = glifos_teste,
        },
        .tamanho_base = FONTE_TESTE_BASE,
        .carregada = false,
    };
    return fonte;
}

static int linha_igual(const LayoutTexto *layout, int indice,
                       const char *esperado) {
    if (layout == NULL || indice < 0 || indice >= layout->quantidade) {
        return 0;
    }

    const LinhaTexto *linha = &layout->linhas[indice];
    int comprimento = (int)strlen(esperado);

    if (linha->comprimento != comprimento) {
        return 0;
    }
    return comprimento == 0 ||
           memcmp(linha->inicio, esperado, (size_t)comprimento) == 0;
}

static void verificar_linhas_dentro_da_largura(const LayoutTexto *layout,
                                               float largura) {
    for (int i = 0; i < layout->quantidade; i++) {
        if (layout->linhas[i].largura > largura + 0.01f) {
            printf("FALHOU: linha %d tem %.2fpx, acima do limite de %.2fpx\n",
                   i, layout->linhas[i].largura, largura);
            falhas++;
        }
    }
}

static void testar_quebra_por_palavra(void) {
    UIFonte fonte = criar_fonte_teste();

    LayoutTexto uma = ui_quebrar_texto(&fonte, "abc abc", 80.0f, 10.0f, 5.0f);
    verificar(uma.quantidade == 1,
              "\"abc abc\" deve caber em uma linha de 80px");
    verificar(linha_igual(&uma, 0, "abc abc"),
              "a linha deve manter as duas palavras");
    verificar(uma.altura == 10.0f,
              "uma linha deve ter altura igual ao tamanho do texto");
    ui_liberar_layout_texto(&uma);

    LayoutTexto duas = ui_quebrar_texto(&fonte, "abc abc", 60.0f, 10.0f, 5.0f);
    verificar(duas.quantidade == 2,
              "\"abc abc\" deve quebrar em duas linhas de 60px");
    verificar(linha_igual(&duas, 0, "abc"),
              "a primeira linha deve ser \"abc\"");
    verificar(linha_igual(&duas, 1, "abc"),
              "a segunda linha deve ser \"abc\"");
    verificar(duas.altura == 25.0f,
              "duas linhas de 10px com 5px de espaço devem somar 25px");
    verificar_linhas_dentro_da_largura(&duas, 60.0f);
    ui_liberar_layout_texto(&duas);
}

static void testar_quebra_com_newline(void) {
    UIFonte fonte = criar_fonte_teste();

    LayoutTexto layout = ui_quebrar_texto(&fonte, "abc\ndef", 100.0f, 10.0f, 5.0f);
    verificar(layout.quantidade == 2,
              "o '\\n' deve criar duas linhas");
    verificar(linha_igual(&layout, 0, "abc"),
              "a linha antes do '\\n' deve ser \"abc\"");
    verificar(linha_igual(&layout, 1, "def"),
              "a linha depois do '\\n' deve ser \"def\"");
    ui_liberar_layout_texto(&layout);

    LayoutTexto paragrafos =
        ui_quebrar_texto(&fonte, "abc\n\ndef", 100.0f, 10.0f, 5.0f);
    verificar(paragrafos.quantidade == 3,
              "duas quebras seguidas devem preservar a linha vazia");
    verificar(paragrafos.linhas[1].comprimento == 0,
              "a linha do meio deve estar vazia");
    verificar(linha_igual(&paragrafos, 2, "def"),
              "o texto deve continuar depois do parágrafo vazio");
    ui_liberar_layout_texto(&paragrafos);
}

static void testar_quebra_de_palavra_longa(void) {
    UIFonte fonte = criar_fonte_teste();

    LayoutTexto layout =
        ui_quebrar_texto(&fonte, "abcdefghij", 45.0f, 10.0f, 5.0f);
    verificar(layout.quantidade == 3,
              "uma palavra maior que a linha deve ser dividida");
    verificar(linha_igual(&layout, 0, "abcd") &&
                  linha_igual(&layout, 1, "efgh") &&
                  linha_igual(&layout, 2, "ij"),
              "a divisão deve respeitar a largura e não perder caracteres");
    verificar_linhas_dentro_da_largura(&layout, 45.0f);
    ui_liberar_layout_texto(&layout);

    LayoutTexto estreito =
        ui_quebrar_texto(&fonte, "abcdefghij", 35.0f, 10.0f, 5.0f);
    verificar(estreito.quantidade == 4,
              "com 35px a palavra deve ocupar quatro linhas");
    verificar(linha_igual(&estreito, 0, "abc") &&
                  linha_igual(&estreito, 3, "j"),
              "o último caractere deve ficar sozinho na última linha");
    verificar_linhas_dentro_da_largura(&estreito, 35.0f);
    ui_liberar_layout_texto(&estreito);
}

static void testar_altura_sem_desenhar(void) {
    UIFonte fonte = criar_fonte_teste();

    // Sem espaçamento informado, o padrão é 25% do tamanho (2,5px).
    float altura =
        ui_medir_altura_texto_quebrado(&fonte, "abcdefghij", 45.0f, 10.0f, 0.0f);
    verificar(altura == 35.0f,
              "três linhas de 10px com 2,5px padrão devem somar 35px");

    LayoutTexto vazio = ui_quebrar_texto(&fonte, "", 100.0f, 10.0f, 5.0f);
    verificar(vazio.quantidade == 0 && vazio.linhas == NULL,
              "texto vazio não deve gerar linhas");
    ui_liberar_layout_texto(&vazio);

    LayoutTexto sem_largura = ui_quebrar_texto(&fonte, "abc", 0.0f, 10.0f, 5.0f);
    verificar(sem_largura.quantidade == 0,
              "largura zero não deve gerar linhas");
    ui_liberar_layout_texto(&sem_largura);

    LayoutTexto so_espacos = ui_quebrar_texto(&fonte, "   ", 100.0f, 10.0f, 5.0f);
    verificar(so_espacos.quantidade == 1 &&
                  so_espacos.linhas[0].comprimento == 0,
              "texto só com espaços deve gerar uma linha vazia");
    ui_liberar_layout_texto(&so_espacos);
}

int main(void) {
    testar_codepoints_do_portugues();
    testar_arquivo_da_fonte();
    testar_glifos_acentuados();
    testar_fonte_sem_acentos();
    testar_quebra_por_palavra();
    testar_quebra_com_newline();
    testar_quebra_de_palavra_longa();
    testar_altura_sem_desenhar();

    if (falhas == 0) {
        printf("OK: todos os testes de interface passaram\n");
        return 0;
    }

    printf("FALHAS: %d\n", falhas);
    return 1;
}
