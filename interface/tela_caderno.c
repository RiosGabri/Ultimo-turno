
#include "tela_caderno.h"
#include "paleta.h"
#include <stdio.h>

#define LARGURA_CADERNO 1280
#define ALTURA_CADERNO 720
#define QUANTIDADE_ABAS TOTAL_SECOES

#define COR_VERMELHO COR_TIJOLO

#define DURACAO_ALERTA_CONTRADICAO 4.0f

static float tempo_alerta_contradicao = 0.0f;
static int quantidade_contradicoes_anterior = -1;
static Caderno *caderno_monitorado = NULL;

static const char *ROTULOS_ABAS[QUANTIDADE_ABAS] = {
    "Depoimentos",
    "Pistas",
    "Reações",
    "Contradições"
};

static void desenhar_contradicao(
    const UIFonte *fonte,
    const Contradicao *contradicao,
    Rectangle painel
) {
    if (contradicao == NULL ||
        contradicao->depoimento1 == NULL ||
        contradicao->depoimento2 == NULL) {
        ui_desenhar_texto_centralizado(
            fonte,
            "Contradição incompleta.",
            painel,
            20,
            COR_TEXTO
        );
        return;
    }

    const Depoimento *a = contradicao->depoimento1;
    const Depoimento *b = contradicao->depoimento2;

    Rectangle painel_a = {
        painel.x,
        painel.y,
        (painel.width - 20) / 2,
        painel.height - 35
    };

    Rectangle painel_b = {
        painel.x + painel_a.width + 20,
        painel.y,
        painel_a.width,
        painel.height - 35
    };

    char texto_a[1024];
    char texto_b[1024];

    snprintf(
        texto_a,
        sizeof(texto_a),
        "Depoimento A | Dia %d\n\n%s",
        a->dia,
        a->texto != NULL ? a->texto : "Sem texto."
    );

    snprintf(
        texto_b,
        sizeof(texto_b),
        "Depoimento B | Dia %d\n\n%s",
        b->dia,
        b->texto != NULL ? b->texto : "Sem texto."
    );

    ui_desenhar_painel_texto(
        fonte,
        painel_a,
        texto_a,
        17,
        COR_PAINEL,
        COR_VERMELHO,
        COR_TEXTO
    );

    ui_desenhar_painel_texto(
        fonte,
        painel_b,
        texto_b,
        17,
        COR_PAINEL,
        COR_VERMELHO,
        COR_TEXTO
    );

    char local_a[256];
    char local_b[256];

    snprintf(
        local_a,
        sizeof(local_a),
        "Local: %s",
        a->local != NULL ? a->local : "Desconhecido"
    );

    snprintf(
        local_b,
        sizeof(local_b),
        "Local: %s",
        b->local != NULL ? b->local : "Desconhecido"
    );

    Rectangle area_local_a = {
        painel_a.x + 10,
        painel_a.y + painel_a.height - 30,
        painel_a.width - 20,
        25
    };

    Rectangle area_local_b = {
        painel_b.x + 10,
        painel_b.y + painel_b.height - 30,
        painel_b.width - 20,
        25
    };

    ui_desenhar_texto_centralizado(
        fonte,
        local_a,
        area_local_a,
        16,
        COR_VERMELHO
    );

    ui_desenhar_texto_centralizado(
        fonte,
        local_b,
        area_local_b,
        16,
        COR_VERMELHO
    );

    Rectangle area_frase = {
        painel.x,
        painel.y + painel.height - 30,
        painel.width,
        25
    };

    ui_desenhar_texto_centralizado(
        fonte,
        "Quem está mentindo? Você decide.",
        area_frase,
        18,
        COR_TEXTO
    );
}

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

            desenhar_contradicao(fonte, contradicao, painel);

            return;
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

void tela_caderno_iniciar_monitoramento(Caderno *caderno) {
    caderno_monitorado = caderno;
    tempo_alerta_contradicao = 0.0f;

    if (caderno != NULL) {
        quantidade_contradicoes_anterior =
            caderno->contradicoes.tamanho;
    } else {
        quantidade_contradicoes_anterior = -1;
    }
}

bool tela_caderno_atualizar_desenhar(
    const UIFonte *fonte,
    Caderno *caderno
) {
    if (fonte == NULL || caderno == NULL) {
        return false;
    }

    // Reinicia o monitoramento ao receber outro caderno.
    if (caderno_monitorado != caderno) {
        caderno_monitorado = caderno;
        quantidade_contradicoes_anterior = -1;
        tempo_alerta_contradicao = 0.0f;
    }

    int quantidade_atual = caderno->contradicoes.tamanho;

    // Primeira leitura: estabelece a quantidade inicial.
    if (quantidade_contradicoes_anterior == -1) {
        quantidade_contradicoes_anterior = quantidade_atual;
    }
    // Detecta o registro de uma nova contradição.
    else if (quantidade_atual > quantidade_contradicoes_anterior) {
        tempo_alerta_contradicao = DURACAO_ALERTA_CONTRADICAO;
        quantidade_contradicoes_anterior = quantidade_atual;
    }
    // Atualiza o controle caso a quantidade diminua.
    else if (quantidade_atual < quantidade_contradicoes_anterior) {
        quantidade_contradicoes_anterior = quantidade_atual;
    }

    // Reduz o tempo restante a cada quadro.
    if (tempo_alerta_contradicao > 0.0f) {
        tempo_alerta_contradicao -= GetFrameTime();

        if (tempo_alerta_contradicao < 0.0f) {
            tempo_alerta_contradicao = 0.0f;
        }
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
                .area = {200 + i * 220, 110, 200, 48},
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

    // Botões de ordenação: aparecem somente na aba Pistas.
    static int ultima_quantidade_trocas = -1;
    static BotaoUI botoes_ordenacao[3] = {
        {
            .area = {370, 175, 160, 40},
            .rotulo = "Local",
            .habilitado = true,
            .pressionado = false
        },
        {
            .area = {550, 175, 160, 40},
            .rotulo = "Objeto",
            .habilitado = true,
            .pressionado = false
        },
        {
            .area = {730, 175, 180, 40},
            .rotulo = "Dia recente",
            .habilitado = true,
            .pressionado = false
        }
    };

    if (caderno->secao_ativa == SECAO_PISTAS) {

        CriterioOrdenacao criterios[3] = {
            ORDENAR_POR_LOCAL,
            ORDENAR_POR_OBJETO,
            ORDENAR_POR_DIA_RECENTE
        };

        for (int i = 0; i < 3; i++) {

            botoes_ordenacao[i].habilitado =
                (caderno->pistas.tamanho >= 2);

            if (ui_desenhar_botao(fonte, &botoes_ordenacao[i], false)) {

                ultima_quantidade_trocas =
                    ordenar_pistas(caderno, criterios[i]);

                // Após ordenar, retorna à primeira pista.
                caderno->pistas.atual = caderno->pistas.inicio;
            }
        }

    }
    // Área reservada para os registros.
    Rectangle painel = {210, 225, 860, 315};
    desenhar_registro_atual(fonte, caderno, painel);
    if (caderno->secao_ativa == SECAO_PISTAS &&
        ultima_quantidade_trocas >= 0) {

    char mensagem[80];

    snprintf(
        mensagem,
        sizeof(mensagem),
        "Ordenacao concluida: %d trocas",
        ultima_quantidade_trocas
    );

    Rectangle area_mensagem = {780, 270, 270, 25};

    ui_desenhar_texto_centralizado(
        fonte,
        mensagem,
        area_mensagem,
        13,
        COR_TEXTO
    );
    }
        // Botões de navegação entre registros.
        // static preserva o estado dos cliques entre os frames.
        static BotaoUI botao_anterior = {
            .area = {340, 550, 180, 40},
            .rotulo = "Anterior",
            .habilitado = true,
            .pressionado = false
        };

        static BotaoUI botao_proximo = {
            .area = {760, 550, 180, 40},
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

        Rectangle area_indicador = {530, 550, 220, 40};

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
        .area = {510, 610, 260, 40},
        .rotulo = "Voltar à partida",
        .habilitado = true,
        .pressionado = false
    };
    bool solicitou_voltar = ui_desenhar_botao(fonte, &voltar, false);

    // Exibe um aviso temporário ao detectar uma nova contradição.
    if (tempo_alerta_contradicao > 0.0f) {
        Rectangle area_alerta = {390, 665, 500, 40};

        DrawRectangleRec(area_alerta, COR_TIJOLO);

        ui_desenhar_texto_centralizado(
            fonte,
            "Nova contradição registrada!",
            area_alerta,
            20,
            RAYWHITE
        );
    }

    return solicitou_voltar;
}
