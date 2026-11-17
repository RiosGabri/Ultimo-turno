#include "jogo.h"
#include "entidades/aleatorio.h"
#include "entidades/locais.h"

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

// A tabela fixa LOCAL_VERDADEIRO foi removida: a agenda secreta é a única
// fonte de verdade sobre onde cada suspeito está em cada dia e período.

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
    if (jogo == NULL) {
        return NULL;
    }

    // Compatibilidade: o "local verdadeiro" é o local do suspeito na noite
    // do dia atual, que também é o local do crime quando ele é o assassino.
    return local_do_suspeito(jogo, id, jogo->dia.dia_atual, NOITE);
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

static const char *local_falso(const Jogo *jogo, int dia, enum Periodo periodo, int id_alvo) {
    const char *verdade = local_do_suspeito(jogo, id_alvo, dia, periodo);
    if (verdade == NULL) {
        return NULL;
    }

    int total = total_locais();
    if (total < 2) {
        return NULL;
    }

    int indice_verdade = -1;
    for (int i = 0; i < total; i++) {
        const char *nome = nome_local(i);
        if (nome != NULL && strcmp(nome, verdade) == 0) {
            indice_verdade = i;
            break;
        }
    }

    if (indice_verdade < 0) {
        return NULL;
    }

    // O deslocamento fica em 1..total-1, então o local falso nunca é igual ao
    // verdadeiro. Depende só de (dia, período, alvo): assassino e cúmplice
    // contam exatamente a mesma versão falsa.
    int deslocamento = 1 + (dia + (int)periodo + id_alvo) % (total - 1);
    return nome_local((indice_verdade + deslocamento) % total);
}

static const char *expressao_periodo(enum Periodo periodo) {
    switch (periodo) {
        case MANHA: return "pela manhã";
        case TARDE: return "à tarde";
        case NOITE: return "à noite";
        default:    return "no período";
    }
}

static void montar_fala(char *destino, size_t tamanho, int sobre_si,
                        const char *nome_alvo, const char *local,
                        enum Periodo periodo) {
    if (sobre_si) {
        snprintf(destino, tamanho, "Eu estava %s %s %s.",
                 preposicao_do_local(local), local, expressao_periodo(periodo));
    } else {
        snprintf(destino, tamanho, "%s estava %s %s %s.",
                 nome_alvo, preposicao_do_local(local), local,
                 expressao_periodo(periodo));
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

    // O local do crime é o local noturno do assassino, já calculado em
    // gerar_agenda(). Ler a agenda evita uma segunda fonte de verdade.
    const char *local = local_do_crime(jogo, jogo->dia.dia_atual);

    char objeto[128];
    char descricao[256];
    snprintf(objeto, sizeof objeto, "Corpo de %s", vitima->nome);

    if (local != NULL) {
        snprintf(descricao, sizeof descricao,
                 "%s foi encontrado morto na madrugada, %s %s. O assassino continua agindo.",
                 vitima->nome, preposicao_do_local(local), local);
    } else {
        local = "local desconhecido";
        snprintf(descricao, sizeof descricao,
                 "%s foi encontrado morto na madrugada. O assassino continua agindo.",
                 vitima->nome);
    }

    registrar_pista(jogo->caderno, jogo->dia.dia_atual,
                    (char *)local, objeto, descricao, NULL);
}

// ---------------------------------------------------------------------------
// Ciclo de vida
// ---------------------------------------------------------------------------
static void gerar_agenda(Jogo *jogo, unsigned int semente) {
    Aleatorio gerador;
    aleatorio_iniciar(&gerador, semente);

    for (int d = 0; d < TOTAL_DIAS; d++)
        for (int p = 0; p < TOTAL_PERIODOS; p++)
            for (int id = 0; id < TOTAL_SUSPEITOS; id++)
                jogo->agenda.local[d][p][id] =
                    (unsigned char)aleatorio_intervalo(&gerador, total_locais());

    for (int d = 0; d < TOTAL_DIAS; d++) {
        jogo->agenda.local_crime[d] =
            jogo->agenda.local[d][NOITE][jogo->papeis.id_assassino];


        // regra de ouro: pelo menos um inocente no local do crime
        //garantir_testemunha(jogo, d, &estado);
    }
}

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

    memset(jogo->agenda.local, 0, sizeof jogo->agenda.local);
    memset(jogo->agenda.local_crime, 0, sizeof jogo->agenda.local_crime);
    gerar_agenda(jogo, semente);

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

const char *local_do_suspeito(const Jogo *jogo, int id, int dia, enum Periodo periodo) {
    if (jogo == NULL || id < 0 || id >= TOTAL_SUSPEITOS ||
        dia < 1 || dia > TOTAL_DIAS ||
        periodo < MANHA || periodo > NOITE) {
        return NULL;
    }

    unsigned char indice = jogo->agenda.local[dia - 1][periodo][id];
    return nome_local(indice);
}

const char *local_do_crime(const Jogo *jogo, int dia) {
    if (jogo == NULL || dia < 1 || dia > TOTAL_DIAS) {
        return NULL;
    }

    unsigned char indice = jogo->agenda.local_crime[dia - 1];
    return nome_local(indice);
}

static enum TipoReacao reagir(const Jogo *jogo, int dia, enum Periodo periodo, int id_falante, int id_alvo, const char *local_afirmado, const Suspeito *outro) {
    if (jogo == NULL || outro == NULL || local_afirmado == NULL) {
        return NEUTRO;
    }

    int assassino = jogo->papeis.id_assassino;
    int cumplice = jogo->papeis.id_cumplice;

    // O assassino discorda de quem fala dele ou do cúmplice e concorda com
    // quem fala dos outros.
    if (outro->id == assassino) {
        if (id_alvo == assassino || id_alvo == cumplice) {
            return DISCORDA;
        }
        return CONCORDA;
    }

    // O cúmplice concorda sempre com o assassino e discorda de quem aponta
    // para ele (ou para o assassino); nos demais casos não se manifesta.
    if (outro->id == cumplice) {
        if (id_falante == assassino) {
            return CONCORDA;
        }
        if (id_alvo == cumplice || id_alvo == assassino) {
            return DISCORDA;
        }
        return NEUTRO;
    }

    // Inocente: só opina quando tem como saber.
    const char *verdade_alvo = local_do_suspeito(jogo, id_alvo, dia, periodo);
    const char *verdade_outro = local_do_suspeito(jogo, outro->id, dia, periodo);
    if (verdade_alvo == NULL || verdade_outro == NULL) {
        return NEUTRO;
    }

    // Sabe o paradeiro do alvo se é o próprio alvo ou se estava com ele.
    if (outro->id == id_alvo || strcmp(verdade_outro, verdade_alvo) == 0) {
        return (strcmp(local_afirmado, verdade_alvo) == 0) ? CONCORDA : DISCORDA;
    }

    // Estava no local afirmado e não viu o alvo lá.
    if (strcmp(verdade_outro, local_afirmado) == 0) {
        return DISCORDA;
    }

    return NEUTRO;
}

int perguntar_sobre(Jogo *jogo, int id_alvo, enum Periodo periodo) {
    if (jogo == NULL || jogo->resultado != JOGO_EM_ANDAMENTO ||
        jogo->dia.fase_atual != FASE_DEBATE) {
        return -1;
    }

    Suspeito *falante = suspeito_do_turno(jogo);
    Suspeito *alvo = suspeito_por_id(jogo, id_alvo);
    if (falante == NULL || alvo == NULL || alvo->situacao != VIVO) {
        return -1;
    }

    // TODO(issue 13, etapa 3): mentir somente no período noturno.
    int mentindo = falante->papel == ASSASSINO || falante->papel == CUMPLICE;
    const char *local;

    if (mentindo) {
        local = local_falso(jogo, jogo->dia.dia_atual, periodo, alvo->id);
    } else {
        local = local_do_suspeito(jogo, alvo->id, jogo->dia.dia_atual, periodo);
    }

    if (local == NULL) {
        return -1;
    }

    char texto[256];
    montar_fala(texto, sizeof texto, alvo->id == falante->id, alvo->nome,
                local, periodo);

    int id_depoimento = registrar_depoimento(
        jogo->caderno, jogo->dia.dia_atual, falante->id, (char *)local,
        periodo, alvo->id, TESTEMUNHO, texto);

    if (id_depoimento < 0) {
        return -1;
    }

    // Os demais suspeitos vivos reagem ao depoimento.
    NoListaDuplaCircular *no = jogo->suspeitos->inicio;
    if (no != NULL) {
        do {
            Suspeito *outro = (Suspeito *)no->dado;
            if (outro != NULL && outro->situacao == VIVO && outro->id != falante->id) {
                enum TipoReacao reacao = reagir(jogo, jogo->dia.dia_atual, periodo, falante->id, alvo->id, local, outro);

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

        const char *local = local_verdadeiro_do_suspeito(jogo, jogo->papeis.id_assassino);
        if (local != NULL) {
            registrar_pista(jogo->caderno, jogo->dia.dia_atual,
                            (char *)local, "Revelação do cúmplice",
                            jogo->revelacao_cumplice, NULL);
        }

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
static Pista *obter_ultima_pista(const Jogo *jogo) {
    if (jogo == NULL || jogo->caderno == NULL) {
        return NULL;
    }

    NoListaDupla *atual = jogo->caderno->pistas.inicio;
    NoListaDupla *ultima = NULL;

    while (atual != NULL) {
        if (ultima == NULL || atual->id > ultima->id) {
            ultima = atual;
        }

        atual = atual->prox;
    }

    return ultima != NULL ? (Pista *)ultima->dado : NULL;
}

const char *descricao_ultima_pista(const Jogo *jogo) {
    Pista *pista = obter_ultima_pista(jogo);

    return (pista != NULL) ? pista->descricao : "";
}

const char *nome_ultima_pista(const Jogo *jogo) {
    Pista *pista = obter_ultima_pista(jogo);

    return (pista != NULL) ? pista->objeto : "";
}