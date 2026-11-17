#include "entidades/caderno.h"
#include "listas/lista-dupla.h"
#include "listas/lista-simples.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Ids fixos dos suspeitos usados no exemplo D1..D5.
 * Eles são os mesmos que vão em id_falante / id_assunto dos depoimentos.
 */
enum {
    ID_MARPLE = 0,
    ID_POIROT = 1,
    ID_ENGENHEIRO = 2,
    ID_OPERARIA = 3,
    ID_VIGIA = 4,
    ID_GERENTE = 5
};

static int falhas = 0;

static void verificar(int condicao, const char *mensagem) {
    if (!condicao) {
        printf("FALHOU: %s\n", mensagem);
        falhas++;
    }
}

/* Confere os ids dos depoimentos do resultado, na ordem, e os ids dos nós. */
static void verificar_resultado(const Lista *resultado, const int *esperados,
                                int quantidade, const char *mensagem) {
    int encontrados = 0;
    int ok = (resultado != NULL);

    if (ok) {
        for (const NoLista *no = resultado->inicio; no != NULL; no = no->prox) {
            if (encontrados >= quantidade) {
                ok = 0;
                break;
            }

            const Depoimento *depoimento = (const Depoimento *)no->dado;
            if (depoimento == NULL || depoimento->id_depoimento != esperados[encontrados]) {
                ok = 0;
            }
            if (no->id != encontrados + 1) { /* ids do resultado são 1, 2, 3... */
                ok = 0;
            }
            encontrados++;
        }

        if (encontrados != quantidade) {
            ok = 0;
        }
    }

    verificar(ok, mensagem);

    if (!ok) {
        printf("  esperados %d depoimento(s), encontrados %d\n", quantidade, encontrados);
        if (quantidade > 0) {
            printf("  esperados:");
            for (int i = 0; i < quantidade; i++) {
                printf(" %d", esperados[i]);
            }
            printf("\n  obtidos:");
            if (resultado != NULL) {
                for (const NoLista *no = resultado->inicio; no != NULL; no = no->prox) {
                    const Depoimento *depoimento = (const Depoimento *)no->dado;
                    printf(" %d", depoimento->id_depoimento);
                }
            }
            printf("\n");
        }
    }
}

static void filtrar_e_verificar(const Caderno *caderno, int falante, int assunto,
                                int dia, int periodo, const int *esperados,
                                int quantidade, const char *mensagem) {
    CriterioFiltro *criterio = criar_criterio(falante, assunto, dia, periodo);
    verificar(criterio != NULL, "criar_criterio deve alocar o criterio");
    if (criterio == NULL) {
        return;
    }

    Lista *resultado = filtrar_depoimentos((Caderno *)caderno, criterio);
    verificar_resultado(resultado, esperados, quantidade, mensagem);

    liberar_resultado_filtro(resultado);
    free(criterio);
}

/*
 * Exemplo D1..D5 combinado:
 *   D1: Engenheiro fala sobre Engenheiro, dia 1, tarde
 *   D2: Operária fala sobre Engenheiro, dia 1, tarde
 *   D3: Vigia fala sobre Operária, dia 2, noite
 *   D4: Engenheiro fala sobre Gerente, dia 2, manhã
 *   D5: Gerente fala sobre Engenheiro, dia 3, manhã
 */
static Caderno *criar_caderno_exemplo(void) {
    Caderno *caderno = inicializar_caderno(ID_MARPLE);
    if (caderno == NULL) {
        return NULL;
    }

    registrar_depoimento(caderno, 1, ID_ENGENHEIRO, "Fundição", TARDE, ID_ENGENHEIRO,
                         TESTEMUNHO, "Eu vi o vigia rondando a fundição.");
    registrar_depoimento(caderno, 1, ID_OPERARIA, "Refeitório", TARDE, ID_ENGENHEIRO,
                         TESTEMUNHO, "O engenheiro saiu mais cedo ontem.");
    registrar_depoimento(caderno, 2, ID_VIGIA, "Portão", NOITE, ID_OPERARIA,
                         ALIBI, "A operária estava no portão à noite.");
    registrar_depoimento(caderno, 2, ID_ENGENHEIRO, "Escritório", MANHA, ID_GERENTE,
                         RELACAO, "O gerente pediu os relatórios.");
    registrar_depoimento(caderno, 3, ID_GERENTE, "Sala de máquinas", MANHA, ID_ENGENHEIRO,
                         CENA_DO_CRIME, "O engenheiro conhecia bem a máquina.");

    return caderno;
}

static void testar_inicializacao_e_caderno_vazio(void) {
    Caderno *vazio = inicializar_caderno(ID_POIROT);
    verificar(vazio != NULL, "inicializar_caderno deve alocar o caderno");
    if (vazio == NULL) {
        return;
    }

    verificar(vazio->id == ID_POIROT, "caderno guarda o id recebido");
    verificar(vazio->depoimentos.inicio == NULL, "secao de depoimentos começa vazia");
    verificar(vazio->depoimentos.fim == NULL, "fim dos depoimentos começa NULL");
    verificar(vazio->depoimentos.atual == NULL, "atual dos depoimentos começa NULL");
    verificar(vazio->depoimentos.tamanho == 0, "tamanho dos depoimentos começa em 0");
    verificar(vazio->pistas.tamanho == 0, "secao de pistas começa vazia");
    verificar(vazio->reacoes.tamanho == 0, "secao de reacoes começa vazia");
    verificar(vazio->prox_id_depoimento == 1, "primeiro id de depoimento e 1");
    verificar(vazio->prox_id_pista == 1, "primeiro id de pista e 1");
    verificar(vazio->prox_id_reacao == 1, "primeiro id de reacao e 1");

    /* Buscar em caderno vazio não pode travar. */
    verificar(buscar_depoimento_por_id(vazio, 1) == NULL, "buscar em caderno vazio retorna NULL");
    verificar(buscar_depoimento_por_id(vazio, 99) == NULL, "buscar id inexistente em caderno vazio");
    verificar(buscar_depoimento_por_id(NULL, 1) == NULL, "buscar em caderno NULL retorna NULL");

    /* Caderno vazio + qualquer filtro = lista vazia, sem travar. */
    filtrar_e_verificar(vazio, DESABILITADO, DESABILITADO, DESABILITADO, DESABILITADO,
                        NULL, 0, "caderno vazio com criterio vazio retorna lista vazia");
    filtrar_e_verificar(vazio, ID_ENGENHEIRO, DESABILITADO, 1, TARDE,
                        NULL, 0, "caderno vazio com qualquer filtro retorna lista vazia");

    liberar_caderno(vazio);
}

static void testar_criterios(void) {
    CriterioFiltro *vazio = criar_criterio_vazio();
    verificar(vazio != NULL, "criar_criterio_vazio deve alocar");
    if (vazio != NULL) {
        verificar(vazio->falante == DESABILITADO, "criterio vazio: falante desabilitado");
        verificar(vazio->assunto == DESABILITADO, "criterio vazio: assunto desabilitado");
        verificar(vazio->dia == DESABILITADO, "criterio vazio: dia desabilitado");
        verificar(vazio->periodo == DESABILITADO, "criterio vazio: periodo desabilitado");
        free(vazio);
    }

    CriterioFiltro *criterio = criar_criterio(ID_ENGENHEIRO, ID_OPERARIA, 2, NOITE);
    verificar(criterio != NULL, "criar_criterio deve alocar");
    if (criterio != NULL) {
        verificar(criterio->falante == ID_ENGENHEIRO, "criar_criterio copia o falante");
        verificar(criterio->assunto == ID_OPERARIA, "criar_criterio copia o assunto");
        verificar(criterio->dia == 2, "criar_criterio copia o dia");
        verificar(criterio->periodo == NOITE, "criar_criterio copia o periodo");
        free(criterio);
    }
}

static void testar_registro_e_busca_por_id(void) {
    Caderno *caderno = criar_caderno_exemplo();
    verificar(caderno != NULL, "criar_caderno_exemplo deve alocar");
    if (caderno == NULL) {
        return;
    }

    verificar(caderno->depoimentos.tamanho == 5, "exemplo tem 5 depoimentos");
    verificar(caderno->prox_id_depoimento == 6, "proximo id de depoimento e 6");

    /* Ids dos depoimentos e dos nós devem ser sequenciais a partir de 1. */
    int esperado = 1;
    for (NoListaDupla *no = caderno->depoimentos.inicio; no != NULL; no = no->prox) {
        Depoimento *depoimento = (Depoimento *)no->dado;
        verificar(no->id == esperado, "id do no do depoimento e sequencial");
        verificar(depoimento->id_depoimento == esperado, "id_depoimento e sequencial");
        esperado++;
    }
    verificar(esperado == 6, "percorreu os 5 depoimentos");

    for (int id = 1; id <= 5; id++) {
        Depoimento *depoimento = buscar_depoimento_por_id(caderno, id);
        verificar(depoimento != NULL, "buscar_depoimento_por_id encontra id existente");
        if (depoimento != NULL) {
            verificar(depoimento->id_depoimento == id, "busca devolve o depoimento certo");
        }
    }

    verificar(buscar_depoimento_por_id(caderno, 0) == NULL, "buscar id 0 retorna NULL");
    verificar(buscar_depoimento_por_id(caderno, 6) == NULL, "buscar id 6 retorna NULL");
    verificar(buscar_depoimento_por_id(caderno, 99) == NULL, "buscar id 99 retorna NULL");

    liberar_caderno(caderno);
}

static void testar_comparacao_direta(void) {
    Caderno *caderno = criar_caderno_exemplo();
    verificar(caderno != NULL, "criar_caderno_exemplo deve alocar");
    if (caderno == NULL) {
        return;
    }

    Depoimento *d1 = buscar_depoimento_por_id(caderno, 1);
    verificar(d1 != NULL, "D1 deve existir");
    if (d1 != NULL) {
        CriterioFiltro criterio = { DESABILITADO, DESABILITADO, DESABILITADO, DESABILITADO };

        verificar(comparar_depoimentos_criterio(d1, &criterio) == 1,
                  "criterio vazio aceita qualquer depoimento");

        criterio.falante = ID_ENGENHEIRO;
        verificar(comparar_depoimentos_criterio(d1, &criterio) == 1,
                  "D1 passa com falante Engenheiro");

        criterio.falante = ID_OPERARIA;
        verificar(comparar_depoimentos_criterio(d1, &criterio) == 0,
                  "D1 reprova com falante Operaria");

        criterio.falante = DESABILITADO;
        criterio.assunto = ID_ENGENHEIRO;
        verificar(comparar_depoimentos_criterio(d1, &criterio) == 1,
                  "D1 passa com assunto Engenheiro");

        criterio.assunto = ID_VIGIA;
        verificar(comparar_depoimentos_criterio(d1, &criterio) == 0,
                  "D1 reprova com assunto Vigia");

        criterio.assunto = DESABILITADO;
        criterio.dia = 1;
        criterio.periodo = TARDE;
        verificar(comparar_depoimentos_criterio(d1, &criterio) == 1,
                  "D1 passa com dia 1 e tarde");

        criterio.dia = 2;
        verificar(comparar_depoimentos_criterio(d1, &criterio) == 0,
                  "D1 reprova no dia 2");

        criterio.dia = 1;
        criterio.periodo = NOITE;
        verificar(comparar_depoimentos_criterio(d1, &criterio) == 0,
                  "D1 reprova a noite");

        /* Um campo reprovado derruba o depoimento, mesmo que os outros passem. */
        criterio.falante = ID_ENGENHEIRO;
        criterio.assunto = ID_ENGENHEIRO;
        criterio.dia = 1;
        criterio.periodo = NOITE;
        verificar(comparar_depoimentos_criterio(d1, &criterio) == 0,
                  "basta um campo errado para reprovar");
    }

    verificar(comparar_depoimentos_criterio(NULL, NULL) == 0,
              "comparacao com ponteiros NULL retorna 0");

    liberar_caderno(caderno);
}

static void testar_tabela_de_filtros(void) {
    Caderno *caderno = criar_caderno_exemplo();
    verificar(caderno != NULL, "criar_caderno_exemplo deve alocar");
    if (caderno == NULL) {
        return;
    }

    const int falante_engenheiro[] = {1, 4};
    const int assunto_engenheiro[] = {1, 2, 5};
    const int engenheiro_dia1_tarde[] = {1, 2};
    const int todos[] = {1, 2, 3, 4, 5};

    filtrar_e_verificar(caderno, ID_ENGENHEIRO, DESABILITADO, DESABILITADO, DESABILITADO,
                        falante_engenheiro, 2, "falante = Engenheiro -> D1, D4");
    filtrar_e_verificar(caderno, DESABILITADO, ID_ENGENHEIRO, DESABILITADO, DESABILITADO,
                        assunto_engenheiro, 3, "assunto = Engenheiro -> D1, D2, D5");
    filtrar_e_verificar(caderno, DESABILITADO, ID_ENGENHEIRO, 1, TARDE,
                        engenheiro_dia1_tarde, 2, "assunto = Engenheiro, dia 1, tarde -> D1, D2");
    filtrar_e_verificar(caderno, DESABILITADO, DESABILITADO, DESABILITADO, DESABILITADO,
                        todos, 5, "criterio vazio -> todos os 5");
    filtrar_e_verificar(caderno, DESABILITADO, ID_VIGIA, DESABILITADO, DESABILITADO,
                        NULL, 0, "assunto = Vigia -> lista vazia");

    /* Filtros combinados que ninguém satisfaz. */
    filtrar_e_verificar(caderno, ID_ENGENHEIRO, ID_VIGIA, DESABILITADO, DESABILITADO,
                        NULL, 0, "falante Engenheiro e assunto Vigia -> lista vazia");
    filtrar_e_verificar(caderno, DESABILITADO, DESABILITADO, 7, NOITE,
                        NULL, 0, "dia 7 -> lista vazia");

    liberar_caderno(caderno);
}

static void testar_atalhos(void) {
    Caderno *caderno = criar_caderno_exemplo();
    verificar(caderno != NULL, "criar_caderno_exemplo deve alocar");
    if (caderno == NULL) {
        return;
    }

    const int do_engenheiro[] = {1, 4};
    const int sobre_engenheiro[] = {1, 2, 5};

    Lista *do_suspeito = depoimentos_do_suspeito(caderno, ID_ENGENHEIRO);
    verificar_resultado(do_suspeito, do_engenheiro, 2,
                        "depoimentos_do_suspeito liga so o filtro de falante");
    liberar_resultado_filtro(do_suspeito);

    Lista *sobre_suspeito = depoimentos_sobre_suspeito(caderno, ID_ENGENHEIRO);
    verificar_resultado(sobre_suspeito, sobre_engenheiro, 3,
                        "depoimentos_sobre_suspeito liga so o filtro de assunto");
    liberar_resultado_filtro(sobre_suspeito);

    Lista *de_quem_ninguem_fala = depoimentos_sobre_suspeito(caderno, ID_VIGIA);
    verificar_resultado(de_quem_ninguem_fala, NULL, 0,
                        "depoimentos_sobre_suspeito sem resultados -> lista vazia");
    liberar_resultado_filtro(de_quem_ninguem_fala);

    liberar_caderno(caderno);
}

static void testar_resultado_nao_copia_e_nao_libera_depoimentos(void) {
    Caderno *caderno = criar_caderno_exemplo();
    verificar(caderno != NULL, "criar_caderno_exemplo deve alocar");
    if (caderno == NULL) {
        return;
    }

    CriterioFiltro *criterio = criar_criterio_vazio();
    verificar(criterio != NULL, "criar_criterio_vazio deve alocar");
    if (criterio != NULL) {
        Lista *resultado = filtrar_depoimentos(caderno, criterio);
        verificar(resultado != NULL && resultado->inicio != NULL,
                  "filtro vazio devolve lista com itens");

        Depoimento *original = buscar_depoimento_por_id(caderno, 1);
        verificar(original != NULL, "depoimento original deve existir");
        if (resultado != NULL && resultado->inicio != NULL) {
            verificar(resultado->inicio->dado == original,
                      "o no do resultado guarda o ponteiro do depoimento original");
        }

        liberar_resultado_filtro(resultado);

        verificar(buscar_depoimento_por_id(caderno, 1) == original,
                  "liberar o resultado nao libera os depoimentos do caderno");
        verificar(caderno->depoimentos.tamanho == 5,
                  "liberar o resultado mantem o caderno intacto");

        free(criterio);
    }

    /* Caderno NULL ou critério NULL: devolve lista vazia, sem travar. */
    CriterioFiltro qualquer = { DESABILITADO, DESABILITADO, DESABILITADO, DESABILITADO };
    Lista *sem_caderno = filtrar_depoimentos(NULL, &qualquer);
    verificar_resultado(sem_caderno, NULL, 0, "caderno NULL -> lista vazia");
    liberar_resultado_filtro(sem_caderno);

    Lista *sem_criterio = filtrar_depoimentos(caderno, NULL);
    verificar_resultado(sem_criterio, NULL, 0, "criterio NULL -> lista vazia");
    liberar_resultado_filtro(sem_criterio);

    liberar_caderno(caderno);
}

static void testar_reacao_vinculada_ao_depoimento(void) {
    Caderno *caderno = criar_caderno_exemplo();
    verificar(caderno != NULL, "criar_caderno_exemplo deve alocar");
    if (caderno == NULL) {
        return;
    }

    int id_depoimento = registrar_depoimento(caderno, 4, ID_POIROT, "Biblioteca", NOITE,
                                             ID_MARPLE, TESTEMUNHO,
                                             "Depoimento usado para linkar a reacao.");
    verificar(id_depoimento == 6, "registrar_depoimento devolve o id 6");

    registrar_reacao(caderno, 4, id_depoimento, "Miss Marple", DISCORDA);

    verificar(caderno->reacoes.tamanho == 1, "reacao foi registrada");
    verificar(caderno->reacoes.inicio != NULL, "secao de reacoes tem um no");
    if (caderno->reacoes.inicio != NULL) {
        verificar(caderno->reacoes.inicio->id == 1, "id do no de reacao e sequencial");

        Reacao *reacao = (Reacao *)caderno->reacoes.inicio->dado;
        verificar(reacao != NULL, "dado do no de reacao existe");
        if (reacao != NULL) {
            verificar(reacao->id_depoimento == id_depoimento,
                      "reacao guarda o id do depoimento");
            verificar(strcmp(reacao->quem_reagiu, "Miss Marple") == 0,
                      "reacao guarda quem reagiu");
            verificar(reacao->tipo_reacao == DISCORDA, "reacao guarda o tipo");

            Depoimento *depoimento = buscar_depoimento_por_id(caderno, reacao->id_depoimento);
            verificar(depoimento != NULL && depoimento->id_depoimento == id_depoimento,
                      "busca pelo id da reacao fecha o vinculo com o depoimento");
        }
    }

    registrar_reacao(caderno, 4, id_depoimento, "Poirot", CONCORDA);
    verificar(caderno->reacoes.tamanho == 2, "segunda reacao foi registrada");
    verificar(caderno->reacoes.fim != NULL && caderno->reacoes.fim->id == 2,
              "segundo id de reacao e 2");

    liberar_caderno(caderno);
}

static void testar_pista(void) {
    Caderno *caderno = inicializar_caderno(ID_POIROT);
    verificar(caderno != NULL, "inicializar_caderno deve alocar");
    if (caderno == NULL) {
        return;
    }

    ListaDupla *relacionados = criar_lista_dupla();
    verificar(relacionados != NULL, "lista de suspeitos relacionados deve alocar");

    int id_relacionado = ID_VIGIA;
    verificar(lista_dupla_inserir_no_fim(relacionados, ID_VIGIA, &id_relacionado) == 1,
              "suspeito relacionado foi inserido na pista");

    registrar_pista(caderno, 2, "Portão", "Chave", "Chave encontrada no portão.", relacionados);

    verificar(caderno->pistas.tamanho == 1, "pista foi registrada");
    verificar(caderno->pistas.inicio != NULL, "secao de pistas tem um no");
    if (caderno->pistas.inicio != NULL) {
        verificar(caderno->pistas.inicio->id == 1, "id do no de pista e sequencial");

        Pista *pista = (Pista *)caderno->pistas.inicio->dado;
        verificar(pista != NULL, "dado do no de pista existe");
        if (pista != NULL) {
            verificar(pista->dia == 2, "pista guarda o dia");
            verificar(strcmp(pista->local, "Portão") == 0, "pista guarda o local");
            verificar(strcmp(pista->objeto, "Chave") == 0, "pista guarda o objeto");
            verificar(strcmp(pista->descricao, "Chave encontrada no portão.") == 0,
                      "pista guarda a descricao");
            verificar(pista->suspeitos_relacionados == relacionados,
                      "pista guarda a lista de suspeitos relacionados");
        }
    }

    liberar_caderno(caderno);
}

static void testar_contradicao_simples(void) {
    Caderno *caderno = inicializar_caderno(ID_MARPLE);

    verificar(caderno != NULL, "caderno deve ser criado");
    if (caderno == NULL) {
        return;
    }

    registrar_depoimento(caderno, 1, ID_ENGENHEIRO, "Refeitório",
                         NOITE, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento A");

    registrar_depoimento(caderno, 1, ID_OPERARIA, "Almoxarifado",
                         NOITE, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento B");

    verificar(caderno->contradicoes.tamanho == 1,
              "dois depoimentos em locais diferentes devem gerar uma contradicao");

    liberar_caderno(caderno);
}

static void testar_depoimentos_no_mesmo_local(void) {
    Caderno *caderno = inicializar_caderno(ID_MARPLE);

    registrar_depoimento(caderno, 1, ID_ENGENHEIRO, "Refeitório",
                         NOITE, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento A");

    registrar_depoimento(caderno, 1, ID_OPERARIA, "Refeitório",
                         NOITE, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento B");

    verificar(caderno->contradicoes.tamanho == 0,
              "depoimentos no mesmo local nao devem gerar contradicao");

    liberar_caderno(caderno);
}

static void testar_local_nao_informado(void) {
    Caderno *caderno = inicializar_caderno(ID_MARPLE);

    registrar_depoimento(caderno, 1, ID_ENGENHEIRO, "não informado",
                         NOITE, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento A");

    registrar_depoimento(caderno, 1, ID_OPERARIA, "Refeitório",
                         NOITE, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento B");

    verificar(caderno->contradicoes.tamanho == 0,
              "local nao informado nao deve gerar contradicao");

    liberar_caderno(caderno);
}

static void testar_dias_diferentes(void) {
    Caderno *caderno = inicializar_caderno(ID_MARPLE);

    registrar_depoimento(caderno, 1, ID_ENGENHEIRO, "Refeitório",
                         NOITE, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento A");

    registrar_depoimento(caderno, 2, ID_OPERARIA, "Almoxarifado",
                         NOITE, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento B");

    verificar(caderno->contradicoes.tamanho == 0,
              "depoimentos de dias diferentes nao devem gerar contradicao");

    liberar_caderno(caderno);
}

static void testar_periodos_diferentes(void) {
    Caderno *caderno = inicializar_caderno(ID_MARPLE);

    registrar_depoimento(caderno, 1, ID_ENGENHEIRO, "Refeitório",
                         MANHA, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento A");

    registrar_depoimento(caderno, 1, ID_OPERARIA, "Almoxarifado",
                         NOITE, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento B");

    verificar(caderno->contradicoes.tamanho == 0,
              "depoimentos de periodos diferentes nao devem gerar contradicao");

    liberar_caderno(caderno);
}

static void testar_tres_depoimentos_contraditorios(void) {
    Caderno *caderno = inicializar_caderno(ID_MARPLE);

    registrar_depoimento(caderno, 1, ID_ENGENHEIRO, "Refeitório",
                         NOITE, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento A");

    registrar_depoimento(caderno, 1, ID_OPERARIA, "Almoxarifado",
                         NOITE, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento B");

    registrar_depoimento(caderno, 1, ID_MARPLE, "Enfermaria",
                         NOITE, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento C");

    verificar(caderno->contradicoes.tamanho == 3,
              "tres locais diferentes devem gerar tres contradicoes");

    liberar_caderno(caderno);
}

static void testar_contradicao_duplicada(void) {
    Caderno *caderno = inicializar_caderno(ID_MARPLE);

    registrar_depoimento(caderno, 1, ID_ENGENHEIRO, "Refeitório",
                         NOITE, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento A");

    registrar_depoimento(caderno, 1, ID_OPERARIA, "Almoxarifado",
                         NOITE, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento B");

    Depoimento *A = caderno->depoimentos.inicio->dado;
    Depoimento *B = caderno->depoimentos.fim->dado;

    registrar_contradicao(caderno, B, A);

    verificar(caderno->contradicoes.tamanho == 1,
              "o mesmo par nao deve ser registrado duas vezes");

    liberar_caderno(caderno);
}

static void testar_criterio_vazio(void) {
    Caderno *caderno = criar_caderno_exemplo();
    CriterioFiltro *criterio = criar_criterio_vazio();

    Lista *resultado = filtrar_depoimentos(caderno, criterio);

    verificar(resultado != NULL, "filtro vazio deve retornar uma lista");

    if (resultado != NULL) {
        int quantidade = 0;

    for (NoLista *atual = resultado->inicio;
        atual != NULL;
        atual = atual->prox) {
        quantidade++;
    }

    verificar(quantidade == caderno->depoimentos.tamanho,
            "criterio vazio deve retornar todos os depoimentos");
    }

    liberar_resultado_filtro(resultado);
    free(criterio);
    liberar_caderno(caderno);
}

static void testar_caderno_vazio_contradicoes(void) {
    Caderno *caderno = inicializar_caderno(ID_MARPLE);

    verificar(detectar_contradicoes_todas(caderno) == 0,
              "caderno vazio nao deve gerar contradicoes");

    verificar(caderno->contradicoes.tamanho == 0,
              "lista de contradicoes deve permanecer vazia");

    liberar_caderno(caderno);
}

static void testar_integridade_memoria_contradicoes(void) {
    Caderno *caderno = inicializar_caderno(ID_MARPLE);

    registrar_depoimento(caderno, 1, ID_ENGENHEIRO, "Refeitório",
                         NOITE, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento A");

    registrar_depoimento(caderno, 1, ID_OPERARIA, "Almoxarifado",
                         NOITE, ID_ENGENHEIRO, TESTEMUNHO, "Depoimento B");

    verificar(caderno->contradicoes.tamanho == 1,
              "deve existir uma contradicao");

    Contradicao *contradicao = caderno->contradicoes.inicio->dado;

    verificar(contradicao->depoimento1 != NULL,
              "primeiro depoimento deve continuar acessivel");

    verificar(contradicao->depoimento2 != NULL,
              "segundo depoimento deve continuar acessivel");

    verificar(strcmp(contradicao->depoimento1->texto, "Depoimento B") == 0 ||
              strcmp(contradicao->depoimento1->texto, "Depoimento A") == 0,
              "texto do primeiro depoimento deve continuar valido");

    liberar_caderno(caderno);
}

static void testar_detectar_contradicoes_todas(void) {
    Caderno *caderno = inicializar_caderno(ID_MARPLE);

    Depoimento *A = malloc(sizeof(Depoimento));
    Depoimento *B = malloc(sizeof(Depoimento));

    verificar(A != NULL && B != NULL,
              "depoimentos devem ser alocados");

    if (A == NULL || B == NULL) {
        free(A);
        free(B);
        liberar_caderno(caderno);
        return;
    }

    *A = (Depoimento){
        .id_depoimento = 1,
        .dia = 1,
        .id_falante = ID_ENGENHEIRO,
        .id_assunto = ID_ENGENHEIRO,
        .local = malloc(strlen("Refeitório") + 1),
        .periodo = NOITE,
        .categoria = TESTEMUNHO,
        .texto = malloc(strlen("Depoimento A") + 1),
    };

    strcpy(A->local, "Refeitório");
    strcpy(A->texto, "Depoimento A");

    *B = (Depoimento){
        .id_depoimento = 2,
        .dia = 1,
        .id_falante = ID_OPERARIA,
        .id_assunto = ID_ENGENHEIRO,
        .local = malloc(strlen("Almoxarifado") + 1),
        .periodo = NOITE,
        .categoria = TESTEMUNHO,
        .texto = malloc(strlen("Depoimento A") + 1),
    };

    strcpy(B->local, "Almoxarifado");
    strcpy(B->texto, "Depoimento B");

    NoListaDupla *noA = malloc(sizeof(NoListaDupla));
    NoListaDupla *noB = malloc(sizeof(NoListaDupla));

    verificar(noA != NULL && noB != NULL,
              "nos devem ser alocados");

    if (noA == NULL || noB == NULL) {
        free(noA);
        free(noB);
        free(A->local);
        free(A->texto);
        free(B->local);
        free(B->texto);
        free(A);
        free(B);
        liberar_caderno(caderno);
        return;
    }

    *noA = (NoListaDupla){.id = 1, .dado = A, .prox = noB, .ant = NULL};
    *noB = (NoListaDupla){.id = 2, .dado = B, .prox = NULL, .ant = noA};

    caderno->depoimentos.inicio = noA;
    caderno->depoimentos.fim = noB;
    caderno->depoimentos.tamanho = 2;
    caderno->prox_id_depoimento = 3;

    verificar(detectar_contradicoes_todas(caderno) == 1,
              "detectar todas deve encontrar uma contradicao");

    verificar(caderno->contradicoes.tamanho == 1,
              "uma contradicao deve ser registrada");

    liberar_caderno(caderno);
}

static void testar_navegacao_por_secao(void) {
    Caderno *caderno = inicializar_caderno(ID_MARPLE);
    verificar(caderno != NULL, "inicializar_caderno deve alocar");
    if (caderno == NULL) {
        return;
    }

    verificar(caderno->secao_ativa == SECAO_DEPOIMENTOS,
              "caderno comeca na secao de depoimentos");

    // Seção vazia: não há item atual e navegar não quebra.
    selecionar_secao(caderno, SECAO_DEPOIMENTOS);
    verificar(obter_item_atual(caderno) == NULL,
              "secao vazia nao tem item atual");
    avancar_secao(caderno);
    voltar_secao(caderno);
    verificar(obter_item_atual(caderno) == NULL,
              "navegar em secao vazia mantem item atual NULL");

    registrar_depoimento(caderno, 1, ID_ENGENHEIRO, "Fundição", TARDE,
                         ID_ENGENHEIRO, TESTEMUNHO, "Depoimento 1");
    registrar_depoimento(caderno, 1, ID_OPERARIA, "Refeitório", TARDE,
                         ID_ENGENHEIRO, TESTEMUNHO, "Depoimento 2");
    registrar_depoimento(caderno, 2, ID_VIGIA, "Portão", NOITE,
                         ID_OPERARIA, ALIBI, "Depoimento 3");

    selecionar_secao(caderno, SECAO_DEPOIMENTOS);
    verificar(obter_item_atual(caderno) == caderno->depoimentos.inicio,
              "selecionar secao posiciona no primeiro item");

    avancar_secao(caderno);
    verificar(obter_item_atual(caderno)->id == 2,
              "avancar vai para o segundo depoimento");

    avancar_secao(caderno);
    verificar(obter_item_atual(caderno)->id == 3,
              "avancar vai para o terceiro depoimento");

    avancar_secao(caderno);
    verificar(obter_item_atual(caderno)->id == 3,
              "avancar para no ultimo item");

    voltar_secao(caderno);
    verificar(obter_item_atual(caderno)->id == 2,
              "voltar retorna ao item anterior");

    voltar_secao(caderno);
    voltar_secao(caderno);
    verificar(obter_item_atual(caderno)->id == 1,
              "voltar para no primeiro item");

    // As seções são independentes: pistas têm sua própria posição.
    int relacionado = ID_VIGIA;
    ListaDupla *lista_relacionados = criar_lista_dupla();
    verificar(lista_relacionados != NULL,
              "lista de suspeitos relacionados deve alocar");
    if (lista_relacionados != NULL) {
        lista_dupla_inserir_no_fim(lista_relacionados, relacionado, &relacionado);
    }
    registrar_pista(caderno, 3, "Pátio", "Martelo", "Martelo achado no pátio.",
                    lista_relacionados);

    selecionar_secao(caderno, SECAO_PISTAS);
    verificar(obter_item_atual(caderno) == caderno->pistas.inicio,
              "selecionar pistas posiciona na primeira pista");
    verificar(caderno->depoimentos.atual->id == 1,
              "posicao dos depoimentos e preservada ao trocar de secao");

    selecionar_secao(caderno, SECAO_REACOES);
    verificar(obter_item_atual(caderno) == NULL,
              "secao de reacoes vazia nao tem item atual");

    liberar_caderno(caderno);
}

static Caderno *criar_caderno_ordenacao(void) {
    Caderno *caderno = inicializar_caderno(ID_POIROT);
    if (caderno == NULL) {
        return NULL;
    }

    // Ordem de registro: local/objeto fora de ordem alfabética e dias 3, 1, 2, 1.
    registrar_pista(caderno, 3, "Fundicao", "Luva", "Luva na fundicao.", NULL);
    registrar_pista(caderno, 1, "Portao", "Chave", "Chave no portao.", NULL);
    registrar_pista(caderno, 2, "Refeitorio", "Xicara", "Xicara no refeitorio.", NULL);
    registrar_pista(caderno, 1, "Escritorio", "Carta", "Carta no escritorio.", NULL);

    return caderno;
}

static void verificar_secao_pistas_integra(const Caderno *caderno) {
    if (caderno == NULL) {
        verificar(0, "caderno NULL nao tem secao de pistas");
        return;
    }

    const Secao *secao = &caderno->pistas;
    if (secao->tamanho == 0) {
        verificar(secao->inicio == NULL && secao->fim == NULL,
                  "secao de pistas vazia deve ter inicio e fim NULL");
        return;
    }

    verificar(secao->inicio != NULL && secao->fim != NULL,
              "secao de pistas nao vazia deve ter inicio e fim");
    if (secao->inicio == NULL || secao->fim == NULL) {
        return;
    }

    verificar(secao->inicio->ant == NULL,
              "inicio da secao de pistas nao deve ter anterior");
    verificar(secao->fim->prox == NULL,
              "fim da secao de pistas nao deve ter proximo");

    int quantidade = 0;
    NoListaDupla *anterior = NULL;
    for (NoListaDupla *no = secao->inicio; no != NULL; no = no->prox) {
        verificar(no->dado != NULL, "pista da secao nao pode ser NULL");
        verificar(no->ant == anterior,
                  "link ant incorreto na secao de pistas");
        if (anterior != NULL) {
            verificar(anterior->prox == no,
                      "link prox incorreto na secao de pistas");
        }
        anterior = no;
        quantidade++;
        if (quantidade > secao->tamanho + 1) {
            verificar(0, "loop detectado na secao de pistas");
            return;
        }
    }

    verificar(quantidade == secao->tamanho,
              "quantidade de nos da secao difere do tamanho");
    verificar(anterior == secao->fim,
              "ultimo no da secao de pistas deve ser o fim");
}

static void verificar_ordem_local(const Caderno *caderno,
                                  const char *esperados[], int quantidade) {
    int posicao = 0;
    for (NoListaDupla *no = caderno->pistas.inicio;
         no != NULL && posicao < quantidade;
         no = no->prox, posicao++) {
        const Pista *pista = (const Pista *)no->dado;
        verificar(pista != NULL && pista->local != NULL &&
                  strcmp(pista->local, esperados[posicao]) == 0,
                  "local da pista fora da ordem esperada");
    }
    verificar(posicao == quantidade,
              "quantidade de pistas ao verificar locais");
}

static void verificar_ordem_objeto(const Caderno *caderno,
                                   const char *esperados[], int quantidade) {
    int posicao = 0;
    for (NoListaDupla *no = caderno->pistas.inicio;
         no != NULL && posicao < quantidade;
         no = no->prox, posicao++) {
        const Pista *pista = (const Pista *)no->dado;
        verificar(pista != NULL && pista->objeto != NULL &&
                  strcmp(pista->objeto, esperados[posicao]) == 0,
                  "objeto da pista fora da ordem esperada");
    }
    verificar(posicao == quantidade,
              "quantidade de pistas ao verificar objetos");
}

static void verificar_ordem_dia(const Caderno *caderno,
                                const int esperados[], int quantidade) {
    int posicao = 0;
    for (NoListaDupla *no = caderno->pistas.inicio;
         no != NULL && posicao < quantidade;
         no = no->prox, posicao++) {
        const Pista *pista = (const Pista *)no->dado;
        verificar(pista != NULL && pista->dia == esperados[posicao],
                  "dia da pista fora da ordem esperada");
    }
    verificar(posicao == quantidade,
              "quantidade de pistas ao verificar dias");
}

static void verificar_ids_pistas(const Caderno *caderno,
                                 const int esperados[], int quantidade) {
    int posicao = 0;
    for (NoListaDupla *no = caderno->pistas.inicio;
         no != NULL && posicao < quantidade;
         no = no->prox, posicao++) {
        verificar(no->id == esperados[posicao],
                  "id do no nao acompanhou a pista ordenada");
    }
    verificar(posicao == quantidade,
              "quantidade de pistas ao verificar ids");
}

static void testar_ordenacao_por_criterios(void) {
    const char *locais_esperados[] = {
        "Escritorio", "Fundicao", "Portao", "Refeitorio"
    };
    const char *objetos_esperados[] = {
        "Carta", "Chave", "Luva", "Xicara"
    };
    const char *objetos_por_dia[] = {
        "Luva", "Xicara", "Chave", "Carta"
    };
    const int dias_esperados[] = {3, 2, 1, 1};
    const int ids_local[] = {4, 1, 2, 3};
    const int ids_objeto[] = {4, 2, 1, 3};
    const int ids_dia[] = {1, 3, 2, 4};

    Caderno *caderno = criar_caderno_ordenacao();
    verificar(caderno != NULL, "caderno de ordenacao por local deve alocar");
    if (caderno != NULL) {
        verificar(caderno->pistas.tamanho == 4,
                  "caderno de ordenacao por local tem quatro pistas");
        int trocas = ordenar_pistas(caderno, ORDENAR_POR_LOCAL);
        verificar(trocas > 0, "ordenar por local deve realizar trocas");
        verificar_ordem_local(caderno, locais_esperados, 4);
        verificar_ids_pistas(caderno, ids_local, 4);
        verificar_secao_pistas_integra(caderno);
        liberar_caderno(caderno);
    }

    caderno = criar_caderno_ordenacao();
    verificar(caderno != NULL, "caderno de ordenacao por objeto deve alocar");
    if (caderno != NULL) {
        int trocas = ordenar_pistas(caderno, ORDENAR_POR_OBJETO);
        verificar(trocas > 0, "ordenar por objeto deve realizar trocas");
        verificar_ordem_objeto(caderno, objetos_esperados, 4);
        verificar_ids_pistas(caderno, ids_objeto, 4);
        verificar_secao_pistas_integra(caderno);
        liberar_caderno(caderno);
    }

    caderno = criar_caderno_ordenacao();
    verificar(caderno != NULL, "caderno de ordenacao por dia deve alocar");
    if (caderno != NULL) {
        int trocas = ordenar_pistas(caderno, ORDENAR_POR_DIA_RECENTE);
        verificar(trocas > 0, "ordenar por dia recente deve realizar trocas");
        verificar_ordem_dia(caderno, dias_esperados, 4);
        verificar_ordem_objeto(caderno, objetos_por_dia, 4);
        verificar_ids_pistas(caderno, ids_dia, 4);
        verificar_secao_pistas_integra(caderno);
        liberar_caderno(caderno);
    }
}

static int coletar_pistas(const Caderno *caderno, Pista *destino[], int maximo) {
    if (caderno == NULL || destino == NULL || maximo <= 0) {
        return 0;
    }

    int quantidade = 0;
    for (NoListaDupla *no = caderno->pistas.inicio;
         no != NULL && quantidade < maximo;
         no = no->prox) {
        destino[quantidade++] = (Pista *)no->dado;
    }
    return quantidade;
}

static int mesma_colecao_de_pistas(Pista *a[], int n, Pista *b[], int m) {
    if (a == NULL || b == NULL || n != m) {
        return 0;
    }

    for (int i = 0; i < n; i++) {
        int ocorrencias = 0;
        for (int j = 0; j < m; j++) {
            if (a[i] == b[j]) {
                ocorrencias++;
            }
        }
        if (ocorrencias != 1) {
            return 0;
        }
    }

    return 1;
}

static void testar_ordenacao_preserva_itens(void) {
    CriterioOrdenacao criterios[] = {
        ORDENAR_POR_LOCAL,
        ORDENAR_POR_OBJETO,
        ORDENAR_POR_DIA_RECENTE
    };

    for (int i = 0; i < 3; i++) {
        Caderno *caderno = criar_caderno_ordenacao();
        verificar(caderno != NULL, "caderno de preservacao das pistas deve alocar");
        if (caderno == NULL) {
            continue;
        }

        Pista *antes[8];
        int total_antes = coletar_pistas(caderno, antes, 8);
        verificar(total_antes == 4, "coleta quatro pistas antes da ordenacao");

        int trocas = ordenar_pistas(caderno, criterios[i]);
        verificar(trocas > 0, "ordenacao de preservacao deve realizar trocas");

        Pista *depois[8];
        int total_depois = coletar_pistas(caderno, depois, 8);
        verificar(total_depois == total_antes,
                  "ordenacao preserva a quantidade de pistas");
        verificar(mesma_colecao_de_pistas(antes, total_antes, depois, total_depois),
                  "ordenacao preserva os mesmos ponteiros de pista");
        verificar(caderno->pistas.tamanho == total_depois,
                  "tamanho da secao bate com a quantidade de nos");
        verificar_secao_pistas_integra(caderno);

        liberar_caderno(caderno);
    }
}

static void testar_ordenacao_chaves_repetidas(void) {
    Caderno *caderno = inicializar_caderno(ID_POIROT);
    verificar(caderno != NULL, "caderno de chaves repetidas deve alocar");
    if (caderno == NULL) {
        return;
    }

    registrar_pista(caderno, 3, "Gerencia", "Chave", "Chave na gerencia.", NULL);
    registrar_pista(caderno, 1, "Fundicao", "Luva", "Primeira luva.", NULL);
    registrar_pista(caderno, 2, "Fundicao", "Luva", "Segunda luva.", NULL);

    verificar(caderno->pistas.tamanho == 3,
              "caderno de chaves repetidas tem tres pistas");
    if (caderno->pistas.tamanho != 3) {
        liberar_caderno(caderno);
        return;
    }

    Pista *gerencia = (Pista *)caderno->pistas.inicio->dado;
    Pista *primeira_luva = (Pista *)caderno->pistas.inicio->prox->dado;
    Pista *segunda_luva = (Pista *)caderno->pistas.fim->dado;

    int trocas = ordenar_pistas(caderno, ORDENAR_POR_LOCAL);
    verificar(trocas > 0, "ordenar chaves repetidas deve realizar trocas");
    verificar(caderno->pistas.tamanho == 3,
              "ordenar chaves repetidas preserva o tamanho");
    verificar(caderno->pistas.inicio->dado == primeira_luva &&
              caderno->pistas.inicio->prox->dado == segunda_luva &&
              caderno->pistas.fim->dado == gerencia,
              "chaves repetidas mantem a ordem estavel e nao duplicam");
    verificar_secao_pistas_integra(caderno);

    liberar_caderno(caderno);
}

static void testar_ordenacao_de_pistas(void) {
    Caderno *caderno = inicializar_caderno(ID_POIROT);
    verificar(caderno != NULL, "inicializar_caderno deve alocar");
    if (caderno == NULL) {
        return;
    }

    // Ordem de registro: dias 3, 1, 2, 1 (ids de nó 1 a 4).
    registrar_pista(caderno, 3, "Fundição", "Luva", "Luva na fundição.", NULL);
    registrar_pista(caderno, 1, "Portão", "Chave", "Chave no portão.", NULL);
    registrar_pista(caderno, 2, "Refeitório", "Xícara", "Xícara no refeitório.", NULL);
    registrar_pista(caderno, 1, "Escritório", "Carta", "Carta no escritório.", NULL);

    ordenar_pistas_por_dia(caderno);

    const int dias_esperados[] = {1, 1, 2, 3};
    const int ids_esperados[] = {2, 4, 3, 1}; // o id acompanha a pista

    int posicao = 0;
    for (const NoListaDupla *no = caderno->pistas.inicio;
         no != NULL && posicao < 4;
         no = no->prox, posicao++) {
        const Pista *pista = (const Pista *)no->dado;
        verificar(pista != NULL && pista->dia == dias_esperados[posicao],
                  "pistas ordenadas por dia crescente");
        verificar(no->id == ids_esperados[posicao],
                  "id do no acompanha a pista apos a ordenacao");
    }
    verificar(posicao == 4, "ordenacao preserva todas as pistas");
    verificar(caderno->pistas.tamanho == 4, "tamanho da secao nao muda");
    verificar(caderno->pistas.fim != NULL &&
              ((const Pista *)caderno->pistas.fim->dado)->dia == 3,
              "ultima pista e a de maior dia");

    // A navegação continua consistente depois da ordenação.
    selecionar_secao(caderno, SECAO_PISTAS);
    verificar(((const Pista *)obter_item_atual(caderno)->dado)->dia == 1,
              "navegacao comeca pela pista de menor dia");

    // Ordenar de novo não altera uma lista já ordenada.
    ordenar_pistas_por_dia(caderno);
    verificar(caderno->pistas.inicio->id == 2 &&
              caderno->pistas.fim->id == 1,
              "ordenacao e estavel quando a lista ja esta ordenada");

    liberar_caderno(caderno);
}

int main(void) {
    testar_inicializacao_e_caderno_vazio();
    testar_criterios();
    testar_registro_e_busca_por_id();
    testar_comparacao_direta();
    testar_tabela_de_filtros();
    testar_atalhos();
    testar_resultado_nao_copia_e_nao_libera_depoimentos();
    testar_reacao_vinculada_ao_depoimento();
    testar_pista();
    testar_contradicao_simples();
    testar_depoimentos_no_mesmo_local();
    testar_local_nao_informado();
    testar_dias_diferentes();
    testar_periodos_diferentes();
    testar_tres_depoimentos_contraditorios();
    testar_contradicao_duplicada();
    testar_criterio_vazio();
    testar_caderno_vazio_contradicoes();
    testar_integridade_memoria_contradicoes();
    testar_detectar_contradicoes_todas();
    testar_navegacao_por_secao();
    testar_ordenacao_de_pistas();
    testar_ordenacao_por_criterios();
    testar_ordenacao_preserva_itens();
    testar_ordenacao_chaves_repetidas();

    if (falhas == 0) {
        printf("OK: todos os testes do caderno passaram\n");
        return 0;
    }

    printf("FALHAS: %d\n", falhas);
    return 1;
}
