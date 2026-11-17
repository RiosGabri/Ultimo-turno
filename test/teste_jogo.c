#include "jogo.h"

#include <stdio.h>
#include <string.h>

static int falhas = 0;

static void verificar(int condicao, const char *mensagem) {
    if (!condicao) {
        printf("FALHOU: %s\n", mensagem);
        falhas++;
    }
}

static int contar_situacao(const Jogo *jogo, enum Situacao situacao) {
    int total = 0;
    const NoListaDuplaCircular *no = jogo->suspeitos->inicio;
    if (no == NULL) {
        return 0;
    }

    do {
        const Suspeito *suspeito = (const Suspeito *)no->dado;
        if (suspeito != NULL && suspeito->situacao == situacao) {
            total++;
        }
        no = no->prox;
    } while (no != jogo->suspeitos->inicio);

    return total;
}

static int id_com_papel(const Jogo *jogo, enum Papel papel) {
    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        const Suspeito *suspeito = buscar_suspeito(jogo, id);
        if (suspeito != NULL && suspeito->papel == papel) {
            return id;
        }
    }
    return NENHUM;
}

static void ir_para_julgamento(Jogo *jogo) {
    if (jogo->dia.fase_atual == FASE_MANHA) {
        avancar_fase_jogo(jogo); // manhã -> debate
    }
    if (jogo->dia.fase_atual == FASE_DEBATE) {
        avancar_fase_jogo(jogo); // debate -> julgamento
    }
}

static void testar_inicializacao(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_HERCULE_POIROT, 1234),
              "iniciar_jogo deve montar a partida");

    verificar(jogo.dia.dia_atual == 1, "partida começa no dia 1");
    verificar(jogo.dia.fase_atual == FASE_MANHA, "partida começa na manhã");
    verificar(jogo.credibilidade == CREDIBILIDADE_INICIAL,
              "credibilidade começa em 100");
    verificar(jogo.resultado == JOGO_EM_ANDAMENTO,
              "partida começa em andamento");
    verificar(jogo.id_detetive == ID_HERCULE_POIROT,
              "detetive escolhido é guardado");
    verificar(jogo.caderno != NULL && jogo.caderno->id == ID_HERCULE_POIROT,
              "caderno pertence ao detetive escolhido");
    verificar(contar_situacao(&jogo, VIVO) == TOTAL_SUSPEITOS,
              "todos os 6 suspeitos começam vivos");
    verificar(jogo.caderno->pistas.tamanho == 1,
              "a manhã do dia 1 registra uma pista");
    verificar(jogo.caderno->depoimentos.tamanho == 0,
              "caderno começa sem depoimentos");
    verificar(jogo.turno_atual == NULL,
              "cursor do debate começa vazio");
    verificar(revelacao_do_cumplice(&jogo)[0] == '\0',
              "partida começa sem revelação do cúmplice");
    verificar(revelacao_do_cumplice(NULL)[0] == '\0',
              "revelação de jogo NULL é vazia");

    verificar(jogo.papeis.id_assassino >= 0 &&
              jogo.papeis.id_assassino < TOTAL_SUSPEITOS,
              "assassino sorteado é um suspeito válido");
    verificar(jogo.papeis.id_cumplice >= 0 &&
              jogo.papeis.id_cumplice < TOTAL_SUSPEITOS,
              "cúmplice sorteado é um suspeito válido");
    verificar(jogo.papeis.id_assassino != jogo.papeis.id_cumplice,
              "assassino e cúmplice são diferentes");

    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        const Suspeito *suspeito = buscar_suspeito(&jogo, id);
        verificar(suspeito != NULL &&
                  suspeito->papel == papel_do_suspeito(id, jogo.papeis),
                  "papel do suspeito bate com o sorteio");
    }

    liberar_jogo(&jogo);
}

static void testar_agenda_local_verdadeiro(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 1234),
              "partida inicia");

    const char *local = local_verdadeiro_do_suspeito(&jogo, ID_GERENTE);
    verificar(local != NULL && local[0] != '\0',
              "agenda tem local verdadeiro para suspeito válido");
    verificar(local_verdadeiro_do_suspeito(&jogo, -1) == NULL,
              "id negativo não tem local");
    verificar(local_verdadeiro_do_suspeito(&jogo, TOTAL_SUSPEITOS) == NULL,
              "id inválido não tem local");
    verificar(local_verdadeiro_do_suspeito(NULL, ID_GERENTE) == NULL,
              "jogo NULL não tem local");

    liberar_jogo(&jogo);
}

static void testar_sorteio_reprodutivel(void) {
    Jogo a;
    Jogo b;

    verificar(iniciar_jogo_com_semente(&a, ID_MISS_MARPLE, 77),
              "primeira partida inicia");
    verificar(iniciar_jogo_com_semente(&b, ID_MISS_MARPLE, 77),
              "segunda partida inicia");

    verificar(a.papeis.id_assassino == b.papeis.id_assassino &&
              a.papeis.id_cumplice == b.papeis.id_cumplice,
              "mesma semente sorteia os mesmos papéis");

    liberar_jogo(&a);
    liberar_jogo(&b);
}

static void testar_ciclo_de_debate_e_contradicoes(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 42),
              "partida inicia");
    if (jogo.suspeitos == NULL) {
        return;
    }

    avancar_fase_jogo(&jogo); // manhã -> debate
    verificar(jogo.dia.fase_atual == FASE_DEBATE, "chegou ao debate");
    verificar(!debate_encerrado(&jogo), "debate começa com turnos disponíveis");

    int turnos = 0;
    int ultimo_falante = -1;
    while (suspeito_do_turno(&jogo) != NULL && turnos < TOTAL_SUSPEITOS + 2) {
        Suspeito *falante = suspeito_do_turno(&jogo);
        verificar(falante->situacao == VIVO,
                  "só suspeitos vivos recebem turno");
        verificar(falante->id != ultimo_falante,
                  "o mesmo suspeito não recebe dois turnos seguidos");

        int id_depoimento = perguntar_sobre(&jogo, ID_GERENTE, NOITE);
        verificar(id_depoimento > 0, "pergunta registra um depoimento");

        ultimo_falante = falante->id;
        turnos++;
    }

    verificar(turnos == TOTAL_SUSPEITOS,
              "a volta do debate passa pelos 6 suspeitos vivos");
    verificar(debate_encerrado(&jogo),
              "debate termina depois da volta completa");
    verificar(jogo.interrogados == (1u << TOTAL_SUSPEITOS) - 1u,
              "todos os suspeitos ficam marcados como interrogados");
    verificar(jogo.caderno->depoimentos.tamanho == TOTAL_SUSPEITOS,
              "cada turno gera um depoimento");
    verificar(jogo.caderno->reacoes.tamanho == TOTAL_SUSPEITOS * (TOTAL_SUSPEITOS - 1),
              "cada depoimento recebe reação dos demais vivos");
    // Os 4 inocentes concordam com o local verdadeiro e os 2 culpados contam a
    // mesma versão falsa: cada culpado contradiz cada inocente (2 x 4 = 8).
    verificar(jogo.caderno->contradicoes.tamanho == 8,
              "contradições aparecem entre culpados e inocentes");

    avancar_fase_jogo(&jogo); // debate -> julgamento
    verificar(jogo.dia.fase_atual == FASE_JULGAMENTO,
              "debate encerrado leva ao julgamento");

    liberar_jogo(&jogo);
}

static void testar_debate_pula_mortos(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 7),
              "partida inicia");

    Suspeito *morto = buscar_suspeito(&jogo, ID_VIGIA);
    verificar(morto != NULL, "suspeito existe");
    if (morto != NULL) {
        morto->situacao = MORTO;
    }

    avancar_fase_jogo(&jogo); // manhã -> debate

    // Com um turno disponível, perguntar sobre um morto é recusado.
    int antes = jogo.caderno->depoimentos.tamanho;
    verificar(perguntar_sobre(&jogo, ID_VIGIA, NOITE) == -1,
              "não é possível interrogar sobre um suspeito morto");
    verificar(jogo.caderno->depoimentos.tamanho == antes,
              "pergunta recusada não registra depoimento");

    int turnos = 0;
    int viu_morto = 0;
    while (suspeito_do_turno(&jogo) != NULL && turnos < TOTAL_SUSPEITOS + 2) {
        if (suspeito_do_turno(&jogo)->id == ID_VIGIA) {
            viu_morto = 1;
        }
        verificar(perguntar_sobre(&jogo, ID_GERENTE, NOITE) > 0,
                  "pergunta registra depoimento");
        turnos++;
    }

    verificar(!viu_morto, "suspeito morto não recebe turno");
    verificar(turnos == TOTAL_SUSPEITOS - 1,
              "debate tem um turno por suspeito vivo");

    liberar_jogo(&jogo);
}

static void testar_dia_completo_ate_a_derrota_por_tempo(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 99),
              "partida inicia");

    int mortos_antes = contar_situacao(&jogo, MORTO);

    for (int dia = 1; dia <= TOTAL_DIAS; dia++) {
        verificar(jogo.dia.dia_atual == dia, "dia avança na ordem");
        ir_para_julgamento(&jogo);
        verificar(jogo.dia.fase_atual == FASE_JULGAMENTO,
                  "chegou ao julgamento do dia");

        avancar_fase_jogo(&jogo); // sem prender ninguém

        if (dia < TOTAL_DIAS) {
            verificar(jogo.dia.fase_atual == FASE_NOITE,
                      "dias 1 a 3 terminam na noite");
            if (dia == 1) {
                verificar(jogo.ultima_vitima == NENHUM,
                          "a primeira noite passa sem mortes");
                verificar(contar_situacao(&jogo, MORTO) == mortos_antes,
                          "a primeira noite não mata suspeitos");
            } else {
                verificar(jogo.ultima_vitima != NENHUM,
                          "o assassino ataca depois da primeira noite");

                const Suspeito *vitima = buscar_suspeito(&jogo, jogo.ultima_vitima);
                verificar(vitima != NULL && vitima->situacao == MORTO,
                          "a vítima da noite fica morta");
            }

            avancar_fase_jogo(&jogo); // noite -> manhã seguinte
            verificar(jogo.dia.fase_atual == FASE_MANHA,
                      "a noite leva à manhã seguinte");
            verificar(jogo.dia.dia_atual == dia + 1,
                      "o dia seguinte é o correto");
        } else {
            verificar(jogo.dia.fase_atual == FASE_FINALIZADO,
                      "o dia 4 termina o jogo");
            verificar(jogo.resultado == JOGO_DERROTA_TEMPO,
                      "sem prisão no dia 4, derrota por tempo");
        }
    }

    verificar(contar_situacao(&jogo, MORTO) == mortos_antes + 2,
              "houve crimes nas duas noites após a primeira");
    verificar(contar_situacao(&jogo, VIVO) == 4,
              "quatro suspeitos continuam vivos no fim");
    verificar(jogo.caderno->pistas.tamanho == 6,
              "caderno registra pistas da manhã e dos crimes");

    // Depois de finalizado, avançar não muda mais nada.
    int credibilidade = jogo.credibilidade;
    avancar_fase_jogo(&jogo);
    verificar(jogo.dia.fase_atual == FASE_FINALIZADO &&
              jogo.credibilidade == credibilidade,
              "jogo finalizado não avança mais");

    liberar_jogo(&jogo);
}

static void testar_vitoria_ao_prender_assassino(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_HERCULE_POIROT, 2024),
              "partida inicia");

    ir_para_julgamento(&jogo);
    verificar(jogo.dia.fase_atual == FASE_JULGAMENTO, "chegou ao julgamento");

    int assassino = jogo.papeis.id_assassino;
    verificar(prender_suspeito(&jogo, assassino) == 1,
              "prender o assassino é aceito");
    verificar(jogo.resultado == JOGO_VITORIA, "prender o assassino dá vitória");
    verificar(jogo.dia.fase_atual == FASE_FINALIZADO, "vitória finaliza o jogo");
    verificar(jogo.credibilidade == CREDIBILIDADE_INICIAL,
              "vitória não custa credibilidade");

    const Suspeito *suspeito = buscar_suspeito(&jogo, assassino);
    verificar(suspeito != NULL && suspeito->situacao == PRESO,
              "o assassino fica preso");

    avancar_fase_jogo(&jogo);
    verificar(jogo.dia.fase_atual == FASE_FINALIZADO &&
              jogo.resultado == JOGO_VITORIA,
              "vitória não é desfeita por avanço de fase");

    liberar_jogo(&jogo);
}

static void testar_custo_prisao_inocente(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 77),
              "partida inicia");

    ir_para_julgamento(&jogo);

    int inocente = id_com_papel(&jogo, INOCENTE);
    verificar(inocente != NENHUM, "existe suspeito inocente");

    int credibilidade_antes = jogo.credibilidade;
    verificar(prender_suspeito(&jogo, inocente) == 1,
              "prisão de inocente é aceita");
    verificar(jogo.credibilidade == credibilidade_antes - CUSTO_PRENDER_INOCENTE,
              "prisão de inocente custa 25");
    verificar(jogo.resultado == JOGO_EM_ANDAMENTO,
              "prisão de inocente não vence");
    verificar(jogo.dia.fase_atual == FASE_JULGAMENTO,
              "prisão de inocente não avança a fase");

    const Suspeito *suspeito = buscar_suspeito(&jogo, inocente);
    verificar(suspeito != NULL && suspeito->situacao == PRESO,
              "inocente fica preso");

    liberar_jogo(&jogo);

    Jogo quatro_prisoes;
    verificar(iniciar_jogo_com_semente(&quatro_prisoes, ID_MISS_MARPLE, 77),
              "partida para quatro prisões inicia");
    ir_para_julgamento(&quatro_prisoes);

    int total_presos_inocentes = 0;
    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        const Suspeito *candidato = buscar_suspeito(&quatro_prisoes, id);
        if (candidato == NULL || candidato->papel != INOCENTE) {
            continue;
        }

        verificar(prender_suspeito(&quatro_prisoes, id) == 1,
                  "prisão de cada inocente é aceita");
        total_presos_inocentes++;
        verificar(quatro_prisoes.credibilidade ==
                      CREDIBILIDADE_INICIAL -
                          total_presos_inocentes * CUSTO_PRENDER_INOCENTE,
                  "cada prisão de inocente reduz a credibilidade em 25");
    }

    verificar(total_presos_inocentes == 4,
              "há quatro inocentes que podem ser presos");
    verificar(quatro_prisoes.credibilidade == 0,
              "quatro prisões de inocentes zeram 100 pontos");
    verificar(quatro_prisoes.resultado == JOGO_DERROTA_CREDIBILIDADE,
              "a quarta prisão de inocente causa derrota");

    liberar_jogo(&quatro_prisoes);
}

static void testar_derrota_por_credibilidade(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 11),
              "partida inicia");

    jogo.credibilidade = CUSTO_PRENDER_INOCENTE;
    ir_para_julgamento(&jogo);

    int inocente = id_com_papel(&jogo, INOCENTE);
    verificar(inocente != NENHUM, "existe suspeito inocente");
    verificar(prender_suspeito(&jogo, inocente) == 1,
              "prisão de inocente é aceita");

    verificar(jogo.credibilidade == 0, "credibilidade chega a zero");
    verificar(jogo.resultado == JOGO_DERROTA_CREDIBILIDADE,
              "credibilidade zero dá derrota");
    verificar(jogo.dia.fase_atual == FASE_FINALIZADO,
              "derrota finaliza o jogo");

    liberar_jogo(&jogo);
}

static void testar_prisao_do_cumplice(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 2024),
              "partida inicia");
    ir_para_julgamento(&jogo);

    int cumplice = jogo.papeis.id_cumplice;
    int assassino = jogo.papeis.id_assassino;
    verificar(cumplice != NENHUM, "partida tem cúmplice");
    verificar(cumplice != assassino, "cúmplice é diferente do assassino");

    const Suspeito *suspeito = buscar_suspeito(&jogo, cumplice);
    verificar(suspeito != NULL && suspeito->papel == CUMPLICE,
              "suspeito é o cúmplice");

    int credibilidade_antes = jogo.credibilidade;
    int pistas_antes = jogo.caderno->pistas.tamanho;

    verificar(prender_suspeito(&jogo, cumplice) == 1,
              "prender o cúmplice é aceito");
    verificar(jogo.resultado == JOGO_EM_ANDAMENTO,
              "prender o cúmplice não vence");
    verificar(jogo.dia.fase_atual == FASE_JULGAMENTO,
              "prender o cúmplice não avança a fase");
    verificar(jogo.credibilidade == credibilidade_antes - CUSTO_PRENDER_CUMPLICE,
              "prender o cúmplice custa 15");
    verificar(suspeito->situacao == PRESO, "cúmplice fica preso");
    verificar(jogo.caderno->pistas.tamanho == pistas_antes + 1,
              "revelação entra no caderno como pista");

    const char *local_esperado = local_verdadeiro_do_suspeito(&jogo, assassino);
    verificar(local_esperado != NULL && local_esperado[0] != '\0',
              "agenda do assassino tem local");

    const Pista *pista = (const Pista *)jogo.caderno->pistas.fim->dado;
    verificar(pista != NULL, "pista de revelação existe");
    verificar(pista->dia == jogo.dia.dia_atual,
              "pista de revelação é do dia atual");
    verificar(pista->local != NULL && strcmp(pista->local, local_esperado) == 0,
              "pista revela o local verdadeiro do assassino");
    verificar(pista->descricao != NULL &&
              strstr(pista->descricao, local_esperado) != NULL,
              "descrição da pista contém o local verdadeiro");
    verificar(pista->objeto != NULL && strstr(pista->objeto, "Revelação") != NULL,
              "pista identifica a revelação do cúmplice");

    const char *mensagem = revelacao_do_cumplice(&jogo);
    verificar(mensagem != NULL && mensagem[0] != '\0',
              "jogo guarda a revelação");
    verificar(strstr(mensagem, local_esperado) != NULL,
              "mensagem da revelação contém o local verdadeiro");

    const Suspeito *assassino_vivo = buscar_suspeito(&jogo, assassino);
    verificar(assassino_vivo != NULL && assassino_vivo->situacao == VIVO,
              "assassino continua vivo após prender o cúmplice");

    liberar_jogo(&jogo);
}

static void testar_derrota_por_credibilidade_cumplice(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_HERCULE_POIROT, 2025),
              "partida inicia");
    ir_para_julgamento(&jogo);

    int cumplice = jogo.papeis.id_cumplice;
    verificar(cumplice != NENHUM, "partida tem cúmplice");

    jogo.credibilidade = CUSTO_PRENDER_CUMPLICE;

    verificar(prender_suspeito(&jogo, cumplice) == 1,
              "prender o cúmplice é aceito");
    verificar(jogo.credibilidade == 0,
              "credibilidade chega a zero ao prender o cúmplice");
    verificar(jogo.resultado == JOGO_DERROTA_CREDIBILIDADE,
              "credibilidade zero ao prender o cúmplice dá derrota");
    verificar(jogo.dia.fase_atual == FASE_FINALIZADO,
              "derrota ao prender o cúmplice finaliza o jogo");

    liberar_jogo(&jogo);
}

static void testar_acoes_invalidas(void) {
    Jogo invalido;
    verificar(iniciar_jogo_com_semente(&invalido, 99, 1) == 0,
              "detetive inválido não inicia partida");

    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 5),
              "partida válida inicia");

    verificar(perguntar_sobre(&jogo, ID_GERENTE, NOITE) == -1,
              "não dá para perguntar fora do debate");
    verificar(prender_suspeito(&jogo, ID_GERENTE) == 0,
              "não dá para prender fora do julgamento");

    avancar_fase_jogo(&jogo); // debate
    verificar(perguntar_sobre(&jogo, 999, NOITE) == -1,
              "alvo inexistente é recusado");
    verificar(perguntar_sobre(&jogo, NENHUM, NOITE) == -1,
              "alvo negativo é recusado");
    verificar(prender_suspeito(&jogo, ID_GERENTE) == 0,
              "não dá para prender durante o debate");

    ir_para_julgamento(&jogo);
    verificar(prender_suspeito(&jogo, 999) == 0,
              "não dá para prender suspeito inexistente");

    int inocente = id_com_papel(&jogo, INOCENTE);
    verificar(prender_suspeito(&jogo, inocente) == 1,
              "primeira prisão do inocente é aceita");
    verificar(prender_suspeito(&jogo, inocente) == 0,
              "não dá para prender o mesmo suspeito duas vezes");

    avancar_fase_jogo(&jogo); // julgamento -> noite
    verificar(prender_suspeito(&jogo, ID_GERENTE) == 0,
              "não dá para prender durante a noite");

    liberar_jogo(&jogo);
}

static void testar_reacoes_confirmam_a_verdade(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 42),
              "partida inicia");

    int assassino = jogo.papeis.id_assassino;
    int cumplice = jogo.papeis.id_cumplice;

    avancar_fase_jogo(&jogo); // manhã -> debate

    // Avança até a vez de um dos culpados e pergunta sobre ele mesmo: a
    // resposta é uma mentira sobre o próprio paradeiro.
    int id_depoimento = -1;
    while (suspeito_do_turno(&jogo) != NULL) {
        Suspeito *falante = suspeito_do_turno(&jogo);
        if (falante->id == assassino || falante->id == cumplice) {
            id_depoimento = perguntar_sobre(&jogo, falante->id, NOITE);
            break;
        }
        perguntar_sobre(&jogo, assassino, NOITE);
    }

    verificar(id_depoimento > 0, "culpado depõe sobre si mesmo");

    int confirmacoes_culpadas = 0;
    int confirmacoes_inocentes = 0;
    for (NoListaDupla *no = jogo.caderno->reacoes.inicio;
         no != NULL && id_depoimento > 0; no = no->prox) {
        const Reacao *reacao = (const Reacao *)no->dado;
        if (reacao == NULL || reacao->id_depoimento != id_depoimento ||
            reacao->tipo_reacao != CONCORDA) {
            continue;
        }

        int culpado = 0;
        for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
            const Suspeito *suspeito = buscar_suspeito(&jogo, id);
            if (suspeito != NULL &&
                strcmp(suspeito->nome, reacao->quem_reagiu) == 0) {
                culpado = (id == assassino || id == cumplice);
                break;
            }
        }

        if (culpado) {
            confirmacoes_culpadas++;
        } else {
            confirmacoes_inocentes++;
        }
    }

    verificar(confirmacoes_culpadas == 1,
              "o outro culpado confirma a versão combinada");
    verificar(confirmacoes_inocentes == 0,
              "nenhum inocente confirma a mentira");

    liberar_jogo(&jogo);
}

static void testar_pontuacao_e_texto_final(void) {
    // Cálculo puro: depende apenas dos três parâmetros.
    verificar(calcular_pontuacao(TOTAL_DIAS, 0, 0) == 0,
              "sem dias restantes, sem credibilidade e sem contradições vale 0");
    verificar(calcular_pontuacao(1, CREDIBILIDADE_INICIAL, 0) ==
                  (TOTAL_DIAS - 1) * PONTOS_POR_DIA_RESTANTE +
                  CREDIBILIDADE_INICIAL * PONTOS_POR_CREDIBILIDADE,
              "dias restantes e credibilidade entram na pontuação");
    verificar(calcular_pontuacao(1, 50, 4) - calcular_pontuacao(1, 50, 3) ==
                  PONTOS_POR_CONTRADICAO,
              "cada contradição vale PONTOS_POR_CONTRADICAO");
    verificar(calcular_pontuacao(2, 100, 0) - calcular_pontuacao(3, 100, 0) ==
                  PONTOS_POR_DIA_RESTANTE,
              "usar um dia a mais custa PONTOS_POR_DIA_RESTANTE");
    verificar(calcular_pontuacao(1, 90, 0) - calcular_pontuacao(1, 80, 0) ==
                  10 * PONTOS_POR_CREDIBILIDADE,
              "cada ponto de credibilidade vale PONTOS_POR_CREDIBILIDADE");

    // Limites: entrada fora da faixa não deixa a pontuação negativa.
    verificar(calcular_pontuacao(TOTAL_DIAS, -50, -10) == 0,
              "credibilidade e contradições negativas valem 0");
    verificar(calcular_pontuacao(1, -50, -10) ==
                  calcular_pontuacao(1, 0, 0),
              "credibilidade e contradições negativas não somam pontos");
    verificar(calcular_pontuacao(TOTAL_DIAS + 10, 100, 0) ==
                  calcular_pontuacao(TOTAL_DIAS, 100, 0),
              "dias além do limite contam como TOTAL_DIAS");

    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_HERCULE_POIROT, 2024),
              "partida inicia");
    verificar(jogo.caderno->contradicoes.tamanho == 0,
              "partida começa sem contradições");
    verificar(pontuacao_do_jogo(&jogo) ==
                  calcular_pontuacao(1, CREDIBILIDADE_INICIAL, 0),
              "partida em andamento não recebe bônus de vitória");
    verificar(pontuacao_do_jogo(NULL) == 0, "jogo NULL vale 0");

    ir_para_julgamento(&jogo);
    verificar(prender_suspeito(&jogo, jogo.papeis.id_assassino) == 1,
              "prender o assassino é aceito");
    verificar(jogo.resultado == JOGO_VITORIA, "vitória registrada");
    verificar(pontuacao_do_jogo(&jogo) ==
                  calcular_pontuacao(jogo.dia.dia_atual, jogo.credibilidade, 0) +
                  PONTOS_VITORIA,
              "vitória soma PONTOS_VITORIA");
    verificar(pontuacao_do_jogo(&jogo) >= PONTOS_VITORIA,
              "vitória vale ao menos PONTOS_VITORIA");

    const char *texto_vitoria = texto_final(&jogo);
    verificar(texto_vitoria != NULL && texto_vitoria[0] != '\0',
              "vitória tem texto narrativo");
    verificar(texto_final(NULL) != NULL && texto_final(NULL)[0] == '\0',
              "jogo NULL não tem texto narrativo");
    int pontuacao_vitoria = pontuacao_do_jogo(&jogo);
    liberar_jogo(&jogo);

    // Derrota por credibilidade: texto próprio e sem bônus de vitória.
    Jogo credibilidade;
    verificar(iniciar_jogo_com_semente(&credibilidade, ID_MISS_MARPLE, 11),
              "partida para derrota por credibilidade inicia");
    credibilidade.credibilidade = CUSTO_PRENDER_INOCENTE;
    ir_para_julgamento(&credibilidade);

    int inocente = id_com_papel(&credibilidade, INOCENTE);
    verificar(inocente != NENHUM, "existe suspeito inocente");
    verificar(prender_suspeito(&credibilidade, inocente) == 1,
              "prisão de inocente é aceita");
    verificar(credibilidade.resultado == JOGO_DERROTA_CREDIBILIDADE,
              "credibilidade zero dá derrota");

    const char *texto_credibilidade = texto_final(&credibilidade);
    verificar(texto_credibilidade != NULL && texto_credibilidade[0] != '\0',
              "derrota por credibilidade tem texto narrativo");
    verificar(strcmp(texto_credibilidade, texto_vitoria) != 0,
              "texto da vitória e da derrota por credibilidade são diferentes");
    verificar(pontuacao_do_jogo(&credibilidade) ==
                  calcular_pontuacao(credibilidade.dia.dia_atual,
                                     credibilidade.credibilidade, 0),
              "derrota por credibilidade não soma bônus de vitória");
    liberar_jogo(&credibilidade);

    // Derrota por tempo: texto próprio e pontuação com os dias todos usados.
    Jogo tempo;
    verificar(iniciar_jogo_com_semente(&tempo, ID_MISS_MARPLE, 99),
              "partida para derrota por tempo inicia");

    for (int dia = 1; dia <= TOTAL_DIAS; dia++) {
        ir_para_julgamento(&tempo);
        avancar_fase_jogo(&tempo); // sem prender ninguém

        if (tempo.resultado == JOGO_EM_ANDAMENTO) {
            avancar_fase_jogo(&tempo); // noite -> manhã seguinte
        }
    }

    verificar(tempo.resultado == JOGO_DERROTA_TEMPO, "prazo encerrado dá derrota");
    verificar(tempo.dia.dia_atual == TOTAL_DIAS,
              "derrota por tempo usa todos os dias");

    const char *texto_tempo = texto_final(&tempo);
    verificar(texto_tempo != NULL && texto_tempo[0] != '\0',
              "derrota por tempo tem texto narrativo");
    verificar(strcmp(texto_tempo, texto_vitoria) != 0 &&
                  strcmp(texto_tempo, texto_credibilidade) != 0,
              "os três finais têm textos diferentes");
    verificar(pontuacao_do_jogo(&tempo) ==
                  calcular_pontuacao(TOTAL_DIAS, tempo.credibilidade,
                                     tempo.caderno->contradicoes.tamanho),
              "derrota por tempo não soma bônus de vitória");
    verificar(pontuacao_do_jogo(&tempo) < pontuacao_vitoria,
              "vitória pontua acima da derrota por tempo");
    liberar_jogo(&tempo);
}

static void testar_ultima_pista_apos_ordenacao(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 1234),
              "partida inicia para testar a ultima pista");
    if (jogo.suspeitos == NULL || jogo.caderno == NULL) {
        return;
    }

    ir_para_julgamento(&jogo); // manhã -> debate -> julgamento
    avancar_fase_jogo(&jogo);  // julgamento -> noite (crime)
    avancar_fase_jogo(&jogo);  // noite -> manhã do dia 2 (pista)

    verificar(jogo.dia.dia_atual == 2 && jogo.dia.fase_atual == FASE_MANHA,
              "chegou à manhã do dia 2");
    verificar(jogo.caderno->pistas.tamanho == 2,
              "manhã do dia 2 tem duas pistas após a primeira noite sem mortes");
    verificar(jogo.caderno->pistas.fim != NULL &&
              jogo.caderno->pistas.fim->id == 2,
              "pista mais recente está no fim antes da ordenação");
    verificar(strcmp(nome_ultima_pista(&jogo), "Luva de trabalho") == 0,
              "nome da última pista antes da ordenação");
    verificar(strstr(descricao_ultima_pista(&jogo), "graxa fresca") != NULL,
              "descrição da última pista antes da ordenação");

    int trocas = ordenar_pistas(jogo.caderno, ORDENAR_POR_LOCAL);
    verificar(trocas > 0, "ordenar por local troca as pistas");
    verificar(jogo.caderno->pistas.fim != NULL &&
              jogo.caderno->pistas.fim->id != 2,
              "pista mais recente deixa de ser o fim após ordenar");
    verificar(strcmp(nome_ultima_pista(&jogo), "Luva de trabalho") == 0,
              "nome da última pista continua correto após ordenar");
    verificar(strstr(descricao_ultima_pista(&jogo), "graxa fresca") != NULL,
              "descrição da última pista continua correta após ordenar");

    liberar_jogo(&jogo);
}

int main(void) {
    testar_inicializacao();
    testar_agenda_local_verdadeiro();
    testar_sorteio_reprodutivel();
    testar_ciclo_de_debate_e_contradicoes();
    testar_reacoes_confirmam_a_verdade();
    testar_debate_pula_mortos();
    testar_dia_completo_ate_a_derrota_por_tempo();
    testar_vitoria_ao_prender_assassino();
    testar_custo_prisao_inocente();
    testar_derrota_por_credibilidade();
    testar_prisao_do_cumplice();
    testar_derrota_por_credibilidade_cumplice();
    testar_acoes_invalidas();
    testar_pontuacao_e_texto_final();
    testar_ultima_pista_apos_ordenacao();

    if (falhas == 0) {
        printf("OK: todos os testes do jogo passaram\n");
        return 0;
    }

    printf("FALHAS: %d\n", falhas);
    return 1;
}
