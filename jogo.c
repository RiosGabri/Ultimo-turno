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
    [ID_OPERARIO]   = {"Dias",    "Operário"},
    [ID_ENFERMEIRA] = {"Esteves", "Enfermeira"},
    [ID_VIGIA]      = {"Farias",  "Vigia noturno"},
};

static const char *LOCAIS[] = {
    "Sala de máquinas", "Gerência", "Linha de produção",
    "Vestiário", "Enfermaria"
};
#define TOTAL_LOCAIS ((int)(sizeof(LOCAIS) / sizeof(LOCAIS[0])))

static const struct {
    const char *local;
    const char *objeto;
    const char *descricao;
} PISTAS_DO_DIA[TOTAL_DIAS] = {
    [0] = {
        "Sala de máquinas", "Chave inglesa",
        "Encontrada próxima ao corpo da vítima, na sala de máquinas. É uma das ferramentas utilizadas na manutenção da fábrica. Sua presença no local não é incomum, mas sua proximidade com o corpo lhe chama a atenção. Não apresenta vestígios de sangue ou marcas."
    },  //falta atualizar os dos outros dias
    [1] = {
        "Linha de produção", "Luva de trabalho",
        "Uma luva de trabalho esquecida na linha de produção, com graxa fresca."
    },
    [2] = {
        "Gerência", "Carta rasgada",
        "Uma carta rasgada na gerência. Sobrou o começo de uma ameaça."
    },
    [3] = {
        "Vestiário", "Chave enferrujada",
        "Uma chave enferrujada no vestiário. A fechadura foi forçada."
    },
};

#define LOCAL_FORA_DA_FABRICA "Fora da fábrica"

static const char *const LOCAL_VERDADEIRO[TOTAL_DIAS][TOTAL_SUSPEITOS] = {
    [0] = {
        [ID_GERENTE]    = "Gerência",
        [ID_CAPATAZ]    = "Linha de produção",
        [ID_ENGENHEIRO] = "Sala de máquinas",
        [ID_OPERARIO]   = "Vestiário",
        [ID_ENFERMEIRA] = "Enfermaria",
        [ID_VIGIA]      = LOCAL_FORA_DA_FABRICA,
    },
    [1] = {
        [ID_GERENTE]    = "Gerência",
        [ID_CAPATAZ]    = "Linha de produção",
        [ID_ENGENHEIRO] = "Sala de máquinas",
        [ID_OPERARIO]   = "Linha de produção",
        [ID_ENFERMEIRA] = "Enfermaria",
        [ID_VIGIA]      = "Linha de produção",
    },
    [2] = {
        [ID_GERENTE]    = "Gerência",
        [ID_CAPATAZ]    = "Linha de produção",
        [ID_ENGENHEIRO] = "Sala de máquinas",
        [ID_OPERARIO]   = "Linha de produção",
        [ID_ENFERMEIRA] = "Enfermaria",
        [ID_VIGIA]      = "Linha de produção",
    },
    [3] = {
        [ID_GERENTE]    = "Gerência",
        [ID_CAPATAZ]    = "Linha de produção",
        [ID_ENGENHEIRO] = "Sala de máquinas",
        [ID_OPERARIO]   = "Linha de produção",
        [ID_ENFERMEIRA] = "Enfermaria",
        [ID_VIGIA]      = "Linha de produção",
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

const char *local_verdadeiro_do_suspeito(const Jogo *jogo, int id) {
    if (jogo == NULL || id < 0 || id >= TOTAL_SUSPEITOS) {
        return NULL;
    }

    int dia = jogo->dia.dia_atual - 1;
    if (dia < 0 || dia >= TOTAL_DIAS) {
        return NULL;
    }

    return LOCAL_VERDADEIRO[dia][id];
}

const char *revelacao_do_cumplice(const Jogo *jogo) {
    if (jogo == NULL) {
        return "";
    }
    return jogo->revelacao_cumplice;
}

static int esta_interrogado(const Jogo *jogo, int id) {
    return (jogo->interrogados & (1u << id)) != 0;
}

static const char *preposicao_local(const char *local) {
    if (local != NULL && strcmp(local, "Vestiário") == 0){
        return "no";
    } else{
        return "na";
    }
}
static const char *local_falso(int dia, int id_alvo) {
    const char *verdade = LOCAL_VERDADEIRO[dia - 1][id_alvo];

    for (int i = 0; i < TOTAL_LOCAIS; i ++) {
        if (strcmp(LOCAIS[i], verdade) == 0) {
            int deslocamento = 1 + (dia + id_alvo) % (TOTAL_LOCAIS - 1);
            return LOCAIS[(i + deslocamento) % TOTAL_LOCAIS];
        }
    }
    return LOCAIS[(id_alvo + dia) % TOTAL_LOCAIS];
}

static void montar_fala(char *destino, size_t tamanho, int sobre_si, const char *nome_alvo, const char *local) {
    if (strcmp(local, LOCAL_FORA_DA_FABRICA) == 0) {    //se o local da pessoa for fora da fábrica:
        if (sobre_si) {
            snprintf(destino, tamanho, "Eu não estava na fábrica na noite anterior.");
        } else {
            snprintf(destino, tamanho, "Não vi %s na fábrica na noite anterior.", nome_alvo);
        }
        return;
    }

    if (sobre_si) {
        snprintf(destino, tamanho, "Eu estava %s %s na noite anterior.", preposicao_local(local), local);
    } else {
        snprintf(destino, tamanho, "Vi %s %s %s na noite anterior.", nome_alvo, preposicao_local(local), local);
    }
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

static enum TipoReacao reagir(int dia, int id_alvo, const char *local_afirmado, const Suspeito *outro, SorteioPapeis papeis) {
    int indice = dia - 1;
    int outro_mentindo = outro->id == papeis.id_assassino || outro->id == papeis.id_cumplice;

    if (outro_mentindo) {
        if (strcmp(local_afirmado, local_falso(dia, id_alvo)) == 0){
            return CONCORDA;
        } else{
            return DISCORDA;
        }
    }

    const char *verdade_alvo = LOCAL_VERDADEIRO[indice][id_alvo];
    const char *verdade_outro = LOCAL_VERDADEIRO[indice][outro->id];

    if (outro->id == id_alvo || strcmp(verdade_outro, verdade_alvo) == 0) {
        if (strcmp(local_afirmado, verdade_alvo) == 0){
            return CONCORDA;
        } else{
            return DISCORDA;
        }
    }
    if (strcmp(verdade_outro, local_afirmado) == 0) {
        return DISCORDA;
    }

    return NEUTRO;
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

    //os inocentes falam a verdade, enquanto o assassino e cumplice mentem
    int mentindo = falante->papel == ASSASSINO || falante->papel == CUMPLICE;
    const char *local;

    if(mentindo == 1){
        local = local_falso(jogo->dia.dia_atual, alvo->id);
    } else{
        local = LOCAL_VERDADEIRO[jogo->dia.dia_atual - 1][alvo->id];
    }
    
    char texto[256];
    montar_fala(texto, sizeof texto, alvo->id == falante->id, alvo->nome, local);

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
                enum TipoReacao reacao = reagir(jogo->dia.dia_atual, alvo->id, local, outro, jogo->papeis);

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
static void preparar_revelacao_cumplice(Jogo *jogo, const Suspeito *cumplice) {
    const char *local = local_verdadeiro_do_suspeito(jogo, jogo->papeis.id_assassino);
    if (local == NULL) {
        local = "local desconhecido";
    }

    snprintf(jogo->revelacao_cumplice, sizeof jogo->revelacao_cumplice,
             "Revelação de %s: o local do assassino na noite do crime era %s.",
             cumplice->nome, local);
}

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

    if (suspeito->papel == CUMPLICE) {
        preparar_revelacao_cumplice(jogo, suspeito);
        jogo->credibilidade -= CUSTO_PRENDER_CUMPLICE;
    } else {
        jogo->credibilidade -= CUSTO_PRENDER_INOCENTE;
    }

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

const char * nome_ultima_pista(const Jogo * jogo){
    if(jogo == NULL || jogo->caderno == NULL || jogo->caderno->pistas.fim == NULL){
        return "";
    }
    Pista * pista = (Pista *)jogo->caderno->pistas.fim->dado;
    if(pista != NULL){
        return pista->objeto;
    } else{
        return "";
    }
}
