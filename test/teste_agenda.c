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

static void testar_agenda_local_verdadeiro(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 1234),
              "partida inicia");

    const char *local = local_verdadeiro_do_suspeito(&jogo, ID_GERENTE);
    verificar(local != NULL && local[0] != '\0',
              "agenda tem local verdadeiro para suspeito válido");

    const char *noturno = local_do_suspeito(&jogo, ID_GERENTE,
                                            jogo.dia.dia_atual, NOITE);
    verificar(local != NULL && noturno != NULL &&
              strcmp(local, noturno) == 0,
              "local verdadeiro é o local noturno do dia atual");

    verificar(local_verdadeiro_do_suspeito(&jogo, -1) == NULL,
              "id negativo não tem local");
    verificar(local_verdadeiro_do_suspeito(&jogo, TOTAL_SUSPEITOS) == NULL,
              "id inválido não tem local");
    verificar(local_verdadeiro_do_suspeito(NULL, ID_GERENTE) == NULL,
              "jogo NULL não tem local");

    liberar_jogo(&jogo);
}

static int agendas_iguais(const Jogo *a, const Jogo *b) {
    for (int dia = 1; dia <= TOTAL_DIAS; dia++) {
        const char *crime_a = local_do_crime(a, dia);
        const char *crime_b = local_do_crime(b, dia);
        if (crime_a == NULL || crime_b == NULL || strcmp(crime_a, crime_b) != 0) {
            return 0;
        }

        for (int periodo = MANHA; periodo <= NOITE; periodo++) {
            for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
                const char *local_a =
                    local_do_suspeito(a, id, dia, (enum Periodo)periodo);
                const char *local_b =
                    local_do_suspeito(b, id, dia, (enum Periodo)periodo);
                if (local_a == NULL || local_b == NULL ||
                    strcmp(local_a, local_b) != 0) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

static void testar_acessores_da_agenda(void) {
    Jogo jogo;
    verificar(iniciar_jogo_com_semente(&jogo, ID_MISS_MARPLE, 1234),
              "partida inicia");

    int preenchida = 1;
    for (int dia = 1; dia <= TOTAL_DIAS; dia++) {
        const char *crime = local_do_crime(&jogo, dia);
        const char *noite_assassino =
            local_do_suspeito(&jogo, jogo.papeis.id_assassino, dia, NOITE);
        verificar(crime != NULL && noite_assassino != NULL &&
                  strcmp(crime, noite_assassino) == 0,
                  "local do crime é o local noturno do assassino");

        for (int periodo = MANHA; periodo <= NOITE; periodo++) {
            for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
                const char *local =
                    local_do_suspeito(&jogo, id, dia, (enum Periodo)periodo);
                if (local == NULL || local[0] == '\0') {
                    preenchida = 0;
                }
            }
        }
    }
    verificar(preenchida, "agenda preenche dias, períodos e suspeitos");

    verificar(local_do_suspeito(&jogo, -1, 1, NOITE) == NULL,
              "id negativo é recusado");
    verificar(local_do_suspeito(&jogo, TOTAL_SUSPEITOS, 1, NOITE) == NULL,
              "id inválido é recusado");
    verificar(local_do_suspeito(&jogo, ID_GERENTE, 0, NOITE) == NULL,
              "dia zero é recusado");
    verificar(local_do_suspeito(&jogo, ID_GERENTE, TOTAL_DIAS + 1, NOITE) == NULL,
              "dia além do jogo é recusado");
    verificar(local_do_suspeito(&jogo, ID_GERENTE, 1, (enum Periodo)-1) == NULL,
              "período negativo é recusado");
    verificar(local_do_suspeito(&jogo, ID_GERENTE, 1,
                                (enum Periodo)TOTAL_PERIODOS) == NULL,
              "período além de NOITE é recusado");
    verificar(local_do_suspeito(NULL, ID_GERENTE, 1, NOITE) == NULL,
              "jogo NULL não tem local");

    verificar(local_do_crime(&jogo, 0) == NULL,
              "dia zero não tem crime");
    verificar(local_do_crime(&jogo, TOTAL_DIAS + 1) == NULL,
              "dia além do jogo não tem crime");
    verificar(local_do_crime(NULL, 1) == NULL,
              "jogo NULL não tem crime");

    liberar_jogo(&jogo);
}

static void testar_agenda_reprodutivel(void) {
    Jogo a;
    Jogo b;
    Jogo c;

    verificar(iniciar_jogo_com_semente(&a, ID_MISS_MARPLE, 4242),
              "primeira partida inicia");
    verificar(iniciar_jogo_com_semente(&b, ID_MISS_MARPLE, 4242),
              "segunda partida inicia");
    verificar(iniciar_jogo_com_semente(&c, ID_MISS_MARPLE, 4243),
              "terceira partida inicia");

    verificar(agendas_iguais(&a, &b), "mesma semente gera a mesma agenda");
    verificar(!agendas_iguais(&a, &c),
              "sementes diferentes geram agendas diferentes");

    liberar_jogo(&a);
    liberar_jogo(&b);
    liberar_jogo(&c);
}

int main(void) {
    testar_agenda_local_verdadeiro();
    testar_acessores_da_agenda();
    testar_agenda_reprodutivel();

    if (falhas == 0) {
        printf("OK: todos os testes da agenda passaram\n");
        return 0;
    }

    printf("FALHAS: %d\n", falhas);
    return 1;
}
