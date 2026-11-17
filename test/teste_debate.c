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

static int eh_culpado(const Jogo *jogo, int id) {
    return id == jogo->papeis.id_assassino || id == jogo->papeis.id_cumplice;
}

static void testar_pergunta_por_periodo(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 31),
              "partida inicia");

    avancar_fase_jogo(&jogo); // manhã -> debate

    enum Periodo periodos[3] = { MANHA, TARDE, NOITE };
    const char *expressoes[3] = { "pela manhã", "à tarde", "à noite" };

    for (int i = 0; i < 3; i++) {
        int id_depoimento = perguntar_sobre(&jogo, ID_GERENTE, periodos[i]);
        verificar(id_depoimento > 0,
                  "pergunta por período registra depoimento");

        Depoimento *dep =
            buscar_depoimento_por_id(jogo.caderno, id_depoimento);
        verificar(dep != NULL, "depoimento por período existe");
        if (dep == NULL) {
            continue;
        }

        verificar(dep->periodo == periodos[i],
                  "depoimento guarda o período escolhido");
        verificar(dep->texto != NULL &&
                  strstr(dep->texto, expressoes[i]) != NULL,
                  "fala cita o período escolhido");
    }

    liberar_jogo(&jogo);
}

// Enquanto a etapa 3 estiver adiada, assassino e cúmplice mentem em todos os
// períodos. Este teste fixa esse comportamento e, principalmente, verifica que
// a mentira é sempre diferente da verdade e igual para os dois culpados.
static void testar_culpados_mentem_e_inocentes_dizem_a_verdade(void) {
    enum Periodo periodos[3] = { MANHA, TARDE, NOITE };

    unsigned int sementes = 40;
    for (unsigned int semente = 1; semente <= sementes; semente++) {
        for (int p = 0; p < 3; p++) {
            Jogo jogo;
            verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, semente),
                      "partida inicia");

            avancar_fase_jogo(&jogo); // manhã -> debate

            enum Periodo periodo = periodos[p];
            const char *verdade =
                local_do_suspeito(&jogo, ID_GERENTE, 1, periodo);
            verificar(verdade != NULL, "agenda informa a verdade do alvo");

            const char *mentira_assassino = NULL;
            const char *mentira_cumplice = NULL;

            for (int turno = 0; turno < TOTAL_SUSPEITOS; turno++) {
                if (suspeito_do_turno(&jogo) == NULL) {
                    break;
                }

                int id_depoimento =
                    perguntar_sobre(&jogo, ID_GERENTE, periodo);
                verificar(id_depoimento > 0,
                          "pergunta registra depoimento");
                if (id_depoimento <= 0) {
                    continue;
                }

                Depoimento *dep =
                    buscar_depoimento_por_id(jogo.caderno, id_depoimento);
                verificar(dep != NULL, "depoimento pode ser consultado");
                if (dep == NULL || verdade == NULL) {
                    continue;
                }

                if (eh_culpado(&jogo, dep->id_falante)) {
                    verificar(strcmp(dep->local, verdade) != 0,
                              "culpado não conta a verdade");
                    if (dep->id_falante == jogo.papeis.id_assassino) {
                        mentira_assassino = dep->local;
                    } else {
                        mentira_cumplice = dep->local;
                    }
                } else {
                    verificar(strcmp(dep->local, verdade) == 0,
                              "inocente conta a verdade");
                }
            }

            verificar(mentira_assassino != NULL && mentira_cumplice != NULL,
                      "os dois culpados depõem");
            verificar(mentira_assassino != NULL && mentira_cumplice != NULL &&
                      strcmp(mentira_assassino, mentira_cumplice) == 0,
                      "assassino e cúmplice contam a mesma mentira");

            liberar_jogo(&jogo);
        }
    }
}

static enum TipoReacao reacao_esperada(const Jogo *jogo, const Depoimento *dep,
                                       const Suspeito *outro) {
    int assassino = jogo->papeis.id_assassino;
    int cumplice = jogo->papeis.id_cumplice;

    // Assassino: discorda de quem fala dele ou do cúmplice; concorda nos demais.
    if (outro->id == assassino) {
        return (dep->id_assunto == assassino || dep->id_assunto == cumplice)
            ? DISCORDA
            : CONCORDA;
    }

    // Cúmplice: concorda sempre com o assassino; discorda quando apontam para
    // ele ou para o assassino; nos demais casos fica neutro.
    if (outro->id == cumplice) {
        if (dep->id_falante == assassino) {
            return CONCORDA;
        }
        if (dep->id_assunto == cumplice || dep->id_assunto == assassino) {
            return DISCORDA;
        }
        return NEUTRO;
    }

    // Inocente: só opina quando tem como saber.
    const char *verdade_alvo =
        local_do_suspeito(jogo, dep->id_assunto, dep->dia, dep->periodo);
    const char *verdade_outro =
        local_do_suspeito(jogo, outro->id, dep->dia, dep->periodo);
    if (verdade_alvo == NULL || verdade_outro == NULL) {
        return NEUTRO;
    }

    if (outro->id == dep->id_assunto ||
        strcmp(verdade_outro, verdade_alvo) == 0) {
        return (strcmp(dep->local, verdade_alvo) == 0) ? CONCORDA : DISCORDA;
    }
    if (strcmp(verdade_outro, dep->local) == 0) {
        return DISCORDA;
    }
    return NEUTRO;
}

static void testar_reacoes_seguem_conhecimento(void) {
    int viu_concorda = 0;
    int viu_discorda = 0;
    int viu_neutro = 0;
    int reacoes_conferidas = 0;

    // 200 sementes cobrem sozinhas as combinações de papéis (quem é assassino,
    // cúmplice, alvo e falante) e os três tipos de reação.
    for (unsigned int semente = 1; semente <= 200; semente++) {
        Jogo jogo;
        verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, semente),
                  "partida inicia");

        avancar_fase_jogo(&jogo); // manhã -> debate

        for (int turno = 0; turno < TOTAL_SUSPEITOS; turno++) {
            if (suspeito_do_turno(&jogo) == NULL) {
                break;
            }

            int id_depoimento = perguntar_sobre(&jogo, ID_GERENTE, NOITE);
            verificar(id_depoimento > 0, "pergunta registra depoimento");
            if (id_depoimento <= 0) {
                continue;
            }

            Depoimento *dep =
                buscar_depoimento_por_id(jogo.caderno, id_depoimento);
            verificar(dep != NULL, "depoimento pode ser consultado");
            if (dep == NULL) {
                continue;
            }

            for (NoListaDupla *no = jogo.caderno->reacoes.inicio;
                 no != NULL; no = no->prox) {
                const Reacao *reacao = (const Reacao *)no->dado;
                if (reacao == NULL || reacao->id_depoimento != id_depoimento) {
                    continue;
                }

                const Suspeito *outro = NULL;
                for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
                    const Suspeito *suspeito = buscar_suspeito(&jogo, id);
                    if (suspeito != NULL &&
                        strcmp(suspeito->nome, reacao->quem_reagiu) == 0) {
                        outro = suspeito;
                        break;
                    }
                }

                verificar(outro != NULL, "quem reagiu é um suspeito conhecido");
                if (outro == NULL) {
                    continue;
                }

                enum TipoReacao esperada = reacao_esperada(&jogo, dep, outro);
                verificar(reacao->tipo_reacao == esperada,
                          "reação corresponde à regra de conhecimento");
                reacoes_conferidas++;

                if (reacao->tipo_reacao == CONCORDA) {
                    viu_concorda = 1;
                } else if (reacao->tipo_reacao == DISCORDA) {
                    viu_discorda = 1;
                } else if (reacao->tipo_reacao == NEUTRO) {
                    viu_neutro = 1;
                }
            }
        }

        liberar_jogo(&jogo);
    }

    verificar(reacoes_conferidas > 0, "reações foram conferidas");
    verificar(viu_concorda, "CONCORDA é gerado");
    verificar(viu_discorda, "DISCORDA é gerado");
    verificar(viu_neutro, "NEUTRO é gerado quando falta conhecimento");
}

static void testar_contradicoes_envolvem_culpados(void) {
    int sementes_com_contradicao = 0;

    for (unsigned int semente = 1; semente <= 200; semente++) {
        Jogo jogo;
        verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, semente),
                  "partida inicia");

        avancar_fase_jogo(&jogo); // manhã -> debate

        // Todos os vivos depõem sobre o mesmo alvo, no mesmo dia e período:
        // qualquer divergência é uma contradição de verdade.
        for (int turno = 0; turno < TOTAL_SUSPEITOS; turno++) {
            if (suspeito_do_turno(&jogo) == NULL) {
                break;
            }
            int id = perguntar_sobre(&jogo, ID_GERENTE, NOITE);
            verificar(id > 0, "pergunta registra depoimento");
        }

        int contradicoes = 0;
        for (NoListaDupla *no = jogo.caderno->contradicoes.inicio;
             no != NULL; no = no->prox) {
            const Contradicao *c = (const Contradicao *)no->dado;
            verificar(c != NULL && c->depoimento1 != NULL &&
                      c->depoimento2 != NULL,
                      "contradição tem os dois depoimentos");
            if (c == NULL || c->depoimento1 == NULL ||
                c->depoimento2 == NULL) {
                continue;
            }

            const Depoimento *a = c->depoimento1;
            const Depoimento *b = c->depoimento2;
            contradicoes++;

            verificar(a->id_assunto == b->id_assunto &&
                      a->dia == b->dia && a->periodo == b->periodo,
                      "contradição compara o mesmo alvo, dia e período");

            int a_culpado = eh_culpado(&jogo, a->id_falante);
            int b_culpado = eh_culpado(&jogo, b->id_falante);
            verificar(a_culpado || b_culpado,
                      "toda contradição envolve assassino ou cúmplice");
            verificar(!(!a_culpado && !b_culpado),
                      "dois inocentes nunca se contradizem");
        }

        verificar(contradicoes > 0,
                  "perguntar todos sobre o mesmo alvo gera contradição");
        if (contradicoes > 0) {
            sementes_com_contradicao++;
        }

        liberar_jogo(&jogo);
    }

    verificar(sementes_com_contradicao == 200,
              "as 200 sementes geram contradição na noite");
}

int main(void) {
    testar_pergunta_por_periodo();
    testar_culpados_mentem_e_inocentes_dizem_a_verdade();
    testar_reacoes_seguem_conhecimento();
    testar_contradicoes_envolvem_culpados();

    if (falhas == 0) {
        printf("OK: todos os testes de debate passaram\n");
        return 0;
    }

    printf("FALHAS: %d\n", falhas);
    return 1;
}
