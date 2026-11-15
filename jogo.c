#include "jogo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ---------------------------------------------------------------------------
// Conteúdo base
// ---------------------------------------------------------------------------
static const struct {
    const char *nome;
    const char *cargo;
} DADOS_SUSPEITOS[TOTAL_SUSPEITOS] = {
    [ID_GERENTE]    = {"Almeida", "Gerente"},
    [ID_CAPATAZ]    = {"Bastos",  "Capataz"},
    [ID_ENGENHEIRO] = {"Costa",   "Engenheiro"},
    [ID_OPERARIO]   = {"Dias",    "Operária"},
    [ID_ENFERMEIRA] = {"Esteves", "Enfermeiro"},
    [ID_VIGIA]      = {"Farias",  "Vigia noturno"},
};

static const char *LOCAIS[] = {
    "Fundição", "Refeitório", "Escritório",
    "Sala de máquinas", "Almoxarifado", "Portão"
};
#define TOTAL_LOCAIS ((int)(sizeof(LOCAIS) / sizeof(LOCAIS[0])))

static const struct {
    const char *local;
    const char *objeto;
    const char *descricao;
} PISTAS_DO_DIA[TOTAL_DIAS] = {
    [0] = {
        "Refeitório", "Xícara quebrada",
        "Uma xícara quebrada no refeitório. Alguém saiu às pressas."
    },
    [1] = {
        "Fundição", "Luva de trabalho",
        "Uma luva de trabalho esquecida na fundição, com graxa fresca."
    },
    [2] = {
        "Escritório", "Carta rasgada",
        "Uma carta rasgada no escritório. Sobrou o começo de uma ameaça."
    },
    [3] = {
        "Portão", "Chave enferrujada",
        "Uma chave enferrujada junto ao portão. A fechadura foi forçada."
    },
};

// ---------------------------------------------------------------------------
// Auxiliares internos
// ---------------------------------------------------------------------------
static Suspeito *suspeito_por_id(const Jogo *jogo, int id) {
    if (jogo == NULL || jogo->suspeitos == NULL) {
        return NULL;
    }
    return (Suspeito *)lista_dupla_circular_buscar_por_id(jogo->suspeitos, id);
}

static int esta_interrogado(const Jogo *jogo, int id) {
    return (jogo->interrogados & (1u << id)) != 0;
}

static void avancar_turno(Jogo *jogo) {
    if (jogo == NULL || jogo->suspeitos == NULL || jogo->suspeitos->inicio == NULL) {
        return;
    }

    NoListaDuplaCircular *comeco = (jogo->turno_atual != NULL)
        ? jogo->turno_atual->prox
        : jogo->suspeitos->inicio;

    NoListaDuplaCircular *atual = comeco;
    do {
        Suspeito *suspeito = (Suspeito *)atual->dado;
        if (suspeito != NULL && suspeito->situacao == VIVO &&
            !esta_interrogado(jogo, suspeito->id)) {
            jogo->turno_atual = atual;
            return;
        }
        atual = atual->prox;
    } while (atual != comeco);

    jogo->turno_atual = NULL; // todos os vivos já falaram
}

static void registrar_pista_do_dia(Jogo *jogo) {
    int indice = jogo->dia.dia_atual - 1;
    if (indice < 0 || indice >= TOTAL_DIAS) {
        return;
    }

    registrar_pista(jogo->caderno, jogo->dia.dia_atual,
                    (char *)PISTAS_DO_DIA[indice].local,
                    (char *)PISTAS_DO_DIA[indice].objeto,
                    (char *)PISTAS_DO_DIA[indice].descricao,
                    NULL);
}

static void realizar_crime_noturno(Jogo *jogo) {
    jogo->ultima_vitima = NENHUM;

    Suspeito *candidatos[TOTAL_SUSPEITOS];
    int total = 0;

    NoListaDuplaCircular *no = jogo->suspeitos->inicio;
    if (no == NULL) {
        return;
    }
    do {
        Suspeito *suspeito = (Suspeito *)no->dado;
        if (suspeito != NULL && suspeito->situacao == VIVO &&
            suspeito->papel != ASSASSINO && suspeito->papel != CUMPLICE) {
            candidatos[total++] = suspeito;
        }
        no = no->prox;
    } while (no != jogo->suspeitos->inicio);

    if (total == 0) {
        return; // não há mais inocentes para o assassino atacar
    }

    Suspeito *vitima = candidatos[rand() % total];
    vitima->situacao = MORTO;
    jogo->ultima_vitima = vitima->id;

    char objeto[128];
    char descricao[256];
    snprintf(objeto, sizeof objeto, "Corpo de %s", vitima->nome);
    snprintf(descricao, sizeof descricao,
             "%s foi encontrado morto na madrugada. O assassino continua agindo.",
             vitima->nome);

    registrar_pista(jogo->caderno, jogo->dia.dia_atual,
                    "Cena do crime", objeto, descricao, NULL);
}

// ---------------------------------------------------------------------------
// Ciclo de vida
// ---------------------------------------------------------------------------
int iniciar_jogo_com_semente(Jogo *jogo, int id_detetive, unsigned int semente) {
    if (jogo == NULL || id_detetive < ID_MISS_MARPLE || id_detetive > ID_HERCULE_POIROT) {
        return 0;
    }

    memset(jogo, 0, sizeof *jogo);
    jogo->id_detetive = id_detetive;
    jogo->credibilidade = CREDIBILIDADE_INICIAL;
    jogo->resultado = JOGO_EM_ANDAMENTO;
    jogo->ultima_vitima = NENHUM;
    jogo->papeis.id_assassino = NENHUM;
    jogo->papeis.id_cumplice = NENHUM;

    srand(semente);

    jogo->suspeitos = criar_lista_dupla_circular();
    if (jogo->suspeitos == NULL) {
        return 0;
    }

    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        carregar_suspeito(jogo->suspeitos, id,
                          (char *)DADOS_SUSPEITOS[id].nome,
                          (char *)DADOS_SUSPEITOS[id].cargo,
                          VIVO, INOCENTE);
    }

    if (contar_vivos(jogo->suspeitos) != TOTAL_SUSPEITOS) {
        liberar_suspeitos(jogo->suspeitos);
        jogo->suspeitos = NULL;
        return 0;
    }

    int ids[TOTAL_SUSPEITOS];
    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        ids[id] = id;
    }

    if (!sortear_papeis(ids, TOTAL_SUSPEITOS, &jogo->papeis)) {
        liberar_suspeitos(jogo->suspeitos);
        jogo->suspeitos = NULL;
        return 0;
    }

    NoListaDuplaCircular *no = jogo->suspeitos->inicio;
    do {
        Suspeito *suspeito = (Suspeito *)no->dado;
        suspeito->papel = papel_do_suspeito(suspeito->id, jogo->papeis);
        no = no->prox;
    } while (no != jogo->suspeitos->inicio);

    jogo->caderno = inicializar_caderno(id_detetive);
    if (jogo->caderno == NULL) {
        liberar_suspeitos(jogo->suspeitos);
        jogo->suspeitos = NULL;
        return 0;
    }

    iniciar_dia(&jogo->dia);
    registrar_pista_do_dia(jogo);
    jogo->turno_atual = NULL;
    jogo->interrogados = 0;

    return 1;
}

int iniciar_jogo(Jogo *jogo, int id_detetive) {
    return iniciar_jogo_com_semente(jogo, id_detetive, (unsigned int)time(NULL));
}

void liberar_jogo(Jogo *jogo) {
    if (jogo == NULL) {
        return;
    }

    if (jogo->caderno != NULL) {
        liberar_caderno(jogo->caderno);
        jogo->caderno = NULL;
    }
    if (jogo->suspeitos != NULL) {
        liberar_suspeitos(jogo->suspeitos);
        jogo->suspeitos = NULL;
    }

    jogo->turno_atual = NULL;
}

// ---------------------------------------------------------------------------
// Fases
// ---------------------------------------------------------------------------
void avancar_fase_jogo(Jogo *jogo) {
    if (jogo == NULL || jogo->resultado != JOGO_EM_ANDAMENTO) {
        return;
    }

    avancar_fase(&jogo->dia);

    switch (jogo->dia.fase_atual) {
        case FASE_DEBATE:
            iniciar_debate(jogo);
            break;

        case FASE_NOITE:
            realizar_crime_noturno(jogo);
            break;

        case FASE_MANHA:
            registrar_pista_do_dia(jogo);
            break;

        case FASE_FINALIZADO:
            // Chegou ao fim do dia 4 sem prender o culpado.
            jogo->resultado = JOGO_DERROTA_TEMPO;
            break;

        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// Debate
// ---------------------------------------------------------------------------
void iniciar_debate(Jogo *jogo) {
    if (jogo == NULL) {
        return;
    }

    jogo->interrogados = 0;
    jogo->turno_atual = NULL;
    avancar_turno(jogo);
}

Suspeito *suspeito_do_turno(const Jogo *jogo) {
    if (jogo == NULL || jogo->dia.fase_atual != FASE_DEBATE || jogo->turno_atual == NULL) {
        return NULL;
    }
    return (Suspeito *)jogo->turno_atual->dado;
}

int perguntar_sobre(Jogo *jogo, int id_alvo) {
    if (jogo == NULL || jogo->resultado != JOGO_EM_ANDAMENTO ||
        jogo->dia.fase_atual != FASE_DEBATE) {
        return -1;
    }

    Suspeito *falante = suspeito_do_turno(jogo);
    Suspeito *alvo = suspeito_por_id(jogo, id_alvo);
    if (falante == NULL || alvo == NULL || alvo->situacao != VIVO) {
        return -1;
    }

    // Cada falante tem uma versão diferente do local, então duas pessoas
    // falando sobre o mesmo alvo no mesmo dia geram contradição.
    int indice_local = (jogo->dia.dia_atual * 7 + id_alvo * 3 + falante->id * 5)
                       % TOTAL_LOCAIS;
    const char *local = LOCAIS[indice_local];

    char texto[256];
    if (alvo->id == falante->id) {
        snprintf(texto, sizeof texto,
                 "Eu estava no %s na noite anterior.", local);
    } else {
        snprintf(texto, sizeof texto,
                 "Vi %s no %s na noite anterior.", alvo->nome, local);
    }

    int id_depoimento = registrar_depoimento(
        jogo->caderno, jogo->dia.dia_atual, falante->id, (char *)local,
        NOITE, alvo->id, TESTEMUNHO, texto);

    if (id_depoimento < 0) {
        return -1;
    }

    // Os demais suspeitos vivos reagem ao depoimento.
    NoListaDuplaCircular *no = jogo->suspeitos->inicio;
    if (no != NULL) {
        do {
            Suspeito *outro = (Suspeito *)no->dado;
            if (outro != NULL && outro->situacao == VIVO && outro->id != falante->id) {
                int fator = (jogo->dia.dia_atual * 31 + falante->id * 17 +
                             id_alvo * 13 + outro->id * 7) % 5;

                enum TipoReacao reacao = (fator == 0) ? DISCORDA : CONCORDA;

                // O assassino sempre nega quando o alvo é ele mesmo.
                if (outro->id == jogo->papeis.id_assassino &&
                    id_alvo == jogo->papeis.id_assassino) {
                    reacao = DISCORDA;
                }

                registrar_reacao(jogo->caderno, jogo->dia.dia_atual,
                                 id_depoimento, outro->nome, reacao);
            }
            no = no->prox;
        } while (no != jogo->suspeitos->inicio);
    }

    jogo->interrogados |= 1u << falante->id;
    avancar_turno(jogo);

    return id_depoimento;
}

int debate_encerrado(const Jogo *jogo) {
    return jogo == NULL || jogo->dia.fase_atual != FASE_DEBATE ||
           jogo->turno_atual == NULL;
}

// ---------------------------------------------------------------------------
// Julgamento
// ---------------------------------------------------------------------------
int prender_suspeito(Jogo *jogo, int id) {
    if (jogo == NULL || jogo->resultado != JOGO_EM_ANDAMENTO ||
        jogo->dia.fase_atual != FASE_JULGAMENTO) {
        return 0;
    }

    Suspeito *suspeito = suspeito_por_id(jogo, id);
    if (suspeito == NULL || suspeito->situacao != VIVO) {
        return 0;
    }

    suspeito->situacao = PRESO;

    if (suspeito->papel == ASSASSINO) {
        jogo->resultado = JOGO_VITORIA;
        jogo->dia.fase_atual = FASE_FINALIZADO;
        return 1;
    }

    jogo->credibilidade -= CUSTO_PRENDER_INOCENTE;
    if (jogo->credibilidade <= 0) {
        jogo->credibilidade = 0;
        jogo->resultado = JOGO_DERROTA_CREDIBILIDADE;
        jogo->dia.fase_atual = FASE_FINALIZADO;
    }

    return 1;
}

// ---------------------------------------------------------------------------
// Consultas
// ---------------------------------------------------------------------------
Suspeito *buscar_suspeito(const Jogo *jogo, int id) {
    return suspeito_por_id(jogo, id);
}

int jogo_terminou(const Jogo *jogo) {
    return jogo == NULL || jogo->resultado != JOGO_EM_ANDAMENTO;
}

const char *nome_fase(FaseDia fase) {
    switch (fase) {
        case FASE_MANHA:      return "Manhã";
        case FASE_DEBATE:     return "Debate";
        case FASE_JULGAMENTO: return "Julgamento";
        case FASE_NOITE:      return "Noite";
        case FASE_FINALIZADO: return "Finalizado";
        default:              return "?";
    }
}

const char *descricao_ultima_pista(const Jogo *jogo) {
    if (jogo == NULL || jogo->caderno == NULL || jogo->caderno->pistas.fim == NULL) {
        return "";
    }
    Pista *pista = (Pista *)jogo->caderno->pistas.fim->dado;
    return (pista != NULL) ? pista->descricao : "";
}
