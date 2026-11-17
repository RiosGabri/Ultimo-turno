#include "jogo.h"

#include <stdio.h>

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

// Avança do debate atual até o debate do dia seguinte, sem interrogar ninguém.
static void avancar_para_o_proximo_dia(Jogo *jogo) {
    avancar_fase_jogo(jogo); // debate -> julgamento
    avancar_fase_jogo(jogo); // julgamento -> noite
    avancar_fase_jogo(jogo); // noite -> manhã seguinte
}

static void testar_inicio_avanca_a_cada_dia(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 1234),
              "partida inicia");

    int inicios[TOTAL_DIAS] = { NENHUM, NENHUM, NENHUM, NENHUM };

    for (int dia = 1; dia <= TOTAL_DIAS; dia++) {
        verificar(jogo.dia.dia_atual == dia, "o dia atual avança na ordem");
        verificar(jogo.dia.fase_atual == FASE_MANHA,
                  "o dia começa na manhã");

        avancar_fase_jogo(&jogo); // manhã -> debate
        verificar(jogo.dia.fase_atual == FASE_DEBATE,
                  "a manhã leva ao debate");

        Suspeito *primeiro = suspeito_do_turno(&jogo);
        verificar(primeiro != NULL, "o debate do dia tem um primeiro falante");
        if (primeiro != NULL) {
            inicios[dia - 1] = primeiro->id;
        }

        if (dia < TOTAL_DIAS) {
            avancar_para_o_proximo_dia(&jogo);

            verificar(jogo.dia.dia_atual == dia + 1,
                      "a noite leva ao dia seguinte");
            verificar(jogo.dia.fase_atual == FASE_MANHA,
                      "o dia seguinte começa na manhã");
        }
    }

    for (int dia = 1; dia < TOTAL_DIAS; dia++) {
        verificar(inicios[dia] != NENHUM && inicios[dia - 1] != NENHUM,
                  "todos os dias tiveram um primeiro falante");
        verificar(inicios[dia] != inicios[dia - 1],
                  "o ponto de partida do debate avança a cada dia");
    }

    liberar_jogo(&jogo);
}

static void testar_volta_completa_comecando_no_meio(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 77),
              "partida inicia");

    avancar_fase_jogo(&jogo); // manhã -> debate do dia 1
    Suspeito *primeiro_dia1 = suspeito_do_turno(&jogo);
    verificar(primeiro_dia1 != NULL, "dia 1 tem primeiro falante");

    avancar_para_o_proximo_dia(&jogo);
    avancar_fase_jogo(&jogo); // manhã -> debate do dia 2

    Suspeito *primeiro_dia2 = suspeito_do_turno(&jogo);
    verificar(primeiro_dia2 != NULL, "dia 2 tem primeiro falante");
    verificar(primeiro_dia1 != NULL && primeiro_dia2 != NULL &&
              primeiro_dia1->id != primeiro_dia2->id,
              "o dia 2 começa em outro suspeito");

    int vivos = contar_vivos(jogo.suspeitos);
    int vistos[TOTAL_SUSPEITOS] = { 0 };
    int turnos = 0;

    while (suspeito_do_turno(&jogo) != NULL &&
           turnos < TOTAL_SUSPEITOS + 2) {
        Suspeito *falante = suspeito_do_turno(&jogo);
        verificar(falante->situacao == VIVO,
                  "só suspeitos vivos recebem turno");
        verificar(!vistos[falante->id],
                  "o mesmo suspeito não recebe dois turnos na mesma volta");

        vistos[falante->id] = 1;
        verificar(perguntar_sobre(&jogo, falante->id, NOITE) > 0,
                  "pergunta registra depoimento");
        turnos++;
    }

    verificar(turnos == vivos,
              "a volta cobre todos os vivos, mesmo começando no meio");
    verificar(debate_encerrado(&jogo),
              "o debate encerra depois da volta completa");

    liberar_jogo(&jogo);
}

static void testar_pula_morto_e_preso(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 7),
              "partida inicia");

    Suspeito *morto = buscar_suspeito(&jogo, ID_VIGIA);
    Suspeito *preso = buscar_suspeito(&jogo, ID_CAPATAZ);
    verificar(morto != NULL && preso != NULL,
              "os suspeitos existem");
    if (morto == NULL || preso == NULL) {
        liberar_jogo(&jogo);
        return;
    }

    morto->situacao = MORTO;
    preso->situacao = PRESO;

    avancar_fase_jogo(&jogo); // manhã -> debate

    int viu_morto = 0;
    int viu_preso = 0;
    int turnos = 0;

    while (suspeito_do_turno(&jogo) != NULL &&
           turnos < TOTAL_SUSPEITOS + 2) {
        Suspeito *falante = suspeito_do_turno(&jogo);
        verificar(falante->situacao == VIVO,
                  "só vivos recebem turno");

        if (falante->id == morto->id) {
            viu_morto = 1;
        }
        if (falante->id == preso->id) {
            viu_preso = 1;
        }

        verificar(perguntar_sobre(&jogo, falante->id, NOITE) > 0,
                  "pergunta registra depoimento");
        turnos++;
    }

    verificar(!viu_morto, "suspeito morto não recebe turno");
    verificar(!viu_preso, "suspeito preso não recebe turno");
    verificar(turnos == TOTAL_SUSPEITOS - 2,
              "a volta cobre todos os vivos restantes");
    verificar(contar_vivos(jogo.suspeitos) == TOTAL_SUSPEITOS - 2,
              "contar_vivos considera apenas os vivos");

    liberar_jogo(&jogo);
}

static void testar_so_um_vivo(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 99),
              "partida inicia");

    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        if (id == ID_GERENTE) {
            continue;
        }

        Suspeito *suspeito = buscar_suspeito(&jogo, id);
        verificar(suspeito != NULL, "suspeito existe");
        if (suspeito != NULL) {
            suspeito->situacao = (id % 2 == 0) ? MORTO : PRESO;
        }
    }

    verificar(contar_vivos(jogo.suspeitos) == 1,
              "sobra apenas um suspeito vivo");

    avancar_fase_jogo(&jogo); // manhã -> debate

    Suspeito *unico = suspeito_do_turno(&jogo);
    verificar(unico != NULL && unico->id == ID_GERENTE,
              "o único vivo recebe o turno");
    verificar(perguntar_sobre(&jogo, ID_GERENTE, NOITE) > 0,
              "o único vivo pode depor");
    verificar(debate_encerrado(&jogo),
              "o debate encerra após a fala do único vivo");

    liberar_jogo(&jogo);
}

static void testar_reentrada_no_mesmo_dia_mantem_inicio(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 31),
              "partida inicia");

    avancar_fase_jogo(&jogo); // manhã -> debate

    Suspeito *primeiro = suspeito_do_turno(&jogo);
    verificar(primeiro != NULL, "o debate tem um primeiro falante");
    if (primeiro == NULL) {
        liberar_jogo(&jogo);
        return;
    }

    int id_inicio = primeiro->id;
    verificar(perguntar_sobre(&jogo, id_inicio, NOITE) > 0,
              "pergunta registra depoimento");

    iniciar_debate(&jogo); // chamada repetida no mesmo dia

    Suspeito *reinicio = suspeito_do_turno(&jogo);
    verificar(reinicio != NULL && reinicio->id == id_inicio,
              "reiniciar o debate no mesmo dia mantém o início");

    liberar_jogo(&jogo);
}

static void testar_pula_candidato_morto_no_dia_seguinte(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 2024),
              "partida inicia");

    avancar_fase_jogo(&jogo); // manhã -> debate do dia 1
    Suspeito *primeiro = suspeito_do_turno(&jogo);
    verificar(primeiro != NULL, "dia 1 tem primeiro falante");
    if (primeiro == NULL) {
        liberar_jogo(&jogo);
        return;
    }

    avancar_fase_jogo(&jogo); // debate -> julgamento
    int candidato = jogo.proximo_inicio_debate;
    Suspeito *proximo = buscar_suspeito(&jogo, candidato);
    verificar(proximo != NULL, "o próximo início aponta para um suspeito");
    if (proximo == NULL) {
        liberar_jogo(&jogo);
        return;
    }

    // Mata justamente quem abriria o debate do dia 2.
    proximo->situacao = MORTO;

    avancar_fase_jogo(&jogo); // julgamento -> noite
    avancar_fase_jogo(&jogo); // noite -> manhã do dia 2
    avancar_fase_jogo(&jogo); // manhã -> debate do dia 2

    Suspeito *inicio_dia2 = suspeito_do_turno(&jogo);
    verificar(inicio_dia2 != NULL, "dia 2 tem primeiro falante");
    verificar(inicio_dia2 != NULL && inicio_dia2->id != candidato,
              "o debate pula o candidato morto e começa no próximo vivo");
    verificar(contar_situacao(&jogo, MORTO) >= 1,
              "há ao menos um morto no dia 2");

    liberar_jogo(&jogo);
}

int main(void) {
    testar_inicio_avanca_a_cada_dia();
    testar_volta_completa_comecando_no_meio();
    testar_pula_morto_e_preso();
    testar_so_um_vivo();
    testar_reentrada_no_mesmo_dia_mantem_inicio();
    testar_pula_candidato_morto_no_dia_seguinte();

    if (falhas == 0) {
        printf("OK: todos os testes da roda do debate passaram\n");
        return 0;
    }

    printf("FALHAS: %d\n", falhas);
    return 1;
}
