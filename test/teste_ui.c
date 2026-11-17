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

int main(void) {
    testar_codepoints_do_portugues();
    testar_arquivo_da_fonte();
    testar_glifos_acentuados();
    testar_fonte_sem_acentos();

    if (falhas == 0) {
        printf("OK: todos os testes de interface passaram\n");
        return 0;
    }

    printf("FALHAS: %d\n", falhas);
    return 1;
}
