
#include "tela_caderno.h"
#include <stdio.h>

#define LARGURA_CADERNO 960
#define ALTURA_CADERNO 540
#define QUANTIDADE_ABAS TOTAL_SECOES

// Paleta visual noir do Último Turno.
static const Color COR_PAINEL = {32, 32, 32, 255};
static const Color COR_TEXTO = {242, 242, 242, 255};
static const Color COR_VERMELHO = {178, 34, 34, 255};

static const char *ROTULOS_ABAS[QUANTIDADE_ABAS] = {
    "Depoimentos",
    "Pistas",
    "Reações",
    "Contradições"
};

bool tela_caderno_atualizar_desenhar(
    const UIFonte *fonte,
    Caderno *caderno
) {
    if (fonte == NULL || caderno == NULL) {
        return false;
    }

    // Título da tela.
    Rectangle titulo = {0, 25, LARGURA_CADERNO, 60};

    ui_desenhar_texto_centralizado(
        fonte, "Caderno de investigação",
        titulo, 36, RAYWHITE
    );

    // Abas do caderno.
    // Os botões mantêm seu estado entre os quadros.
    static BotaoUI abas[QUANTIDADE_ABAS];
    static bool abas_inicializadas = false;

    if (!abas_inicializadas) {
        for (int i = 0; i < QUANTIDADE_ABAS; i++) {
            abas[i] = (BotaoUI){
                .area = {50 + i * 220, 110, 200, 48},
                .rotulo = ROTULOS_ABAS[i],
                .habilitado = true,
                .pressionado = false
            };
        }

        abas_inicializadas = true;
    }

    // Desenha as abas e processa os cliques.
    for (int i = 0; i < QUANTIDADE_ABAS; i++) {
        bool selecionada = (caderno->secao_ativa == (SecaoCaderno)i);

        if (ui_desenhar_botao(fonte, &abas[i], selecionada)) {
            selecionar_secao(caderno, (SecaoCaderno)i);
        }
    }

    // Área reservada para os registros.
    Rectangle painel = {50, 185, 860, 245};

    DrawRectangleRec(painel, COR_PAINEL);

    DrawRectangleLinesEx(painel, 2, COR_VERMELHO);

    ui_desenhar_texto_centralizado(fonte,"Os registros aparecerão aqui.",painel,22,COR_TEXTO);

    // Botão para retornar à partida.
    static BotaoUI voltar = {
        .area = {350, 455, 260, 55},
        .rotulo = "Voltar à partida",
        .habilitado = true,
        .pressionado = false
    };
    return ui_desenhar_botao(fonte, &voltar, false);
}
