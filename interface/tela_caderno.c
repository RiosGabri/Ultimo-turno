
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

static void desenhar_registro_atual(
    const UIFonte *fonte,
    Caderno *caderno,
    Rectangle painel
) {
    NoListaDupla *no = obter_item_atual(caderno);

    if (no == NULL || no->dado == NULL) {
        ui_desenhar_texto_centralizado(
            fonte,
            "Nenhum registro disponível nesta seção.",
            painel,
            20,
            COR_TEXTO
        );
        return;
    }

    char texto[2048] = {0};

    switch (caderno->secao_ativa) {
        case SECAO_DEPOIMENTOS: {
            Depoimento *depoimento = (Depoimento *)no->dado;

            snprintf(
                texto, sizeof(texto),
                "Depoimento %d | Dia %d\n\n%s",
                depoimento->id_depoimento,
                depoimento->dia,
                depoimento->texto != NULL
                    ? depoimento->texto
                    : "Sem texto."
            );
            break;
        }

        case SECAO_PISTAS: {
            Pista *pista = (Pista *)no->dado;

            snprintf(
                texto, sizeof(texto),
                "Dia %d\nLocal: %s\nObjeto: %s\n\n%s",
                pista->dia,
                pista->local != NULL ? pista->local : "Desconhecido",
                pista->objeto != NULL ? pista->objeto : "Desconhecido",
                pista->descricao != NULL
                    ? pista->descricao
                    : "Sem descrição."
            );
            break;
        }

        case SECAO_REACOES: {
            Reacao *reacao = (Reacao *)no->dado;

            const char *tipo = "Neutro";

            if (reacao->tipo_reacao == CONCORDA) {
                tipo = "Concorda";
            } else if (reacao->tipo_reacao == DISCORDA) {
                tipo = "Discorda";
            }

            snprintf(
                texto, sizeof(texto),
                "Dia %d\nQuem reagiu: %s\nDepoimento: %d\nReação: %s",
                reacao->dia,
                reacao->quem_reagiu != NULL
                    ? reacao->quem_reagiu
                    : "Desconhecido",
                reacao->id_depoimento,
                tipo
            );
            break;
        }

        case SECAO_CONTRADICOES: {
            Contradicao *contradicao = (Contradicao *)no->dado;

            if (contradicao->depoimento1 == NULL ||
                contradicao->depoimento2 == NULL) {
                snprintf(texto, sizeof(texto),
                         "Contradição incompleta.");
                break;
            }

            snprintf(
                texto, sizeof(texto),
                "Contradição entre os depoimentos %d e %d",
                contradicao->depoimento1->id_depoimento,
                contradicao->depoimento2->id_depoimento
            );
            break;
        }

        default:
            return;
    }

   Rectangle area_texto = painel;

    ui_desenhar_painel_texto(
        fonte,
        area_texto,
        texto,
        19,
        COR_PAINEL,
        COR_VERMELHO,
        COR_TEXTO
    );
}

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

    desenhar_registro_atual(fonte, caderno, painel);

    // Botões de navegação entre registros.
    // static preserva o estado dos cliques entre os frames.
    static BotaoUI botao_anterior = {
        .area = {180, 435, 180, 40},
        .rotulo = "Anterior",
        .habilitado = true,
        .pressionado = false
    };

    static BotaoUI botao_proximo = {
        .area = {600, 435, 180, 40},
        .rotulo = "Próximo",
        .habilitado = true,
        .pressionado = false
    };

    // Verifica se existe um registro anterior ou próximo.
    NoListaDupla *atual = obter_item_atual(caderno);

    // Identifica a seção atualmente selecionada.
    Secao *secao = NULL;

    switch (caderno->secao_ativa) {
        case SECAO_DEPOIMENTOS:
            secao = &caderno->depoimentos;
            break;

        case SECAO_PISTAS:
            secao = &caderno->pistas;
            break;

        case SECAO_REACOES:
            secao = &caderno->reacoes;
            break;

        case SECAO_CONTRADICOES:
            secao = &caderno->contradicoes;
            break;

        default:
            break;
    }

    // Calcula a posição do registro atual.
    int posicao = 0;
    int total = 0;

    if (secao != NULL) {
        total = secao->tamanho;

        int indice = 1;

        for (NoListaDupla *no = secao->inicio;
            no != NULL;
            no = no->prox) {

            if (no == atual) {
                posicao = indice;
                break;
            }

            indice++;
        }
    }

    // Exibe o indicador entre os botões de navegação.
    char indicador[64];

    snprintf(
        indicador,
        sizeof(indicador),
        "Registro %d de %d",
        posicao,
        total
    );

    Rectangle area_indicador = {370, 435, 220, 40};

    ui_desenhar_texto_centralizado(
        fonte,
        indicador,
        area_indicador,
        18,
        COR_TEXTO
    );

    botao_anterior.habilitado =
        (atual != NULL && atual->ant != NULL);

    botao_proximo.habilitado =
        (atual != NULL && atual->prox != NULL);

    // Processa os cliques.
    if (ui_desenhar_botao(fonte, &botao_anterior, false)) {
        voltar_secao(caderno);
    }

    if (ui_desenhar_botao(fonte, &botao_proximo, false)) {
        avancar_secao(caderno);
    }
    
    // Botão para retornar à partida.
    static BotaoUI voltar = {
        .area = {350, 490, 260, 40},
        .rotulo = "Voltar à partida",
        .habilitado = true,
        .pressionado = false
    };
    return ui_desenhar_botao(fonte, &voltar, false);
}
