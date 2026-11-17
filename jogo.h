#ifndef JOGO_H
#define JOGO_H

#include "dia.h"
#include "entidades/caderno.h"
#include "entidades/personagens.h"
#include "entidades/suspeito.h"
#include "listas/lista-dupla-circular.h"

#define CREDIBILIDADE_INICIAL 100
#define CUSTO_PRENDER_INOCENTE 30
#define CUSTO_PRENDER_CUMPLICE 15

typedef enum {
    JOGO_EM_ANDAMENTO,
    JOGO_VITORIA,
    JOGO_DERROTA_CREDIBILIDADE,
    JOGO_DERROTA_TEMPO
} ResultadoJogo;

/*
 * Estado completo de uma partida. O motor não depende de interface:
 * a UI de terminal (e, depois, a da raylib) só lê este estado e chama
 * as funções de ação abaixo.
 */
typedef struct {
    int id_detetive;
    EstadoDia dia;
    ListaDuplaCircular *suspeitos;
    SorteioPapeis papeis;
    Caderno *caderno;
    int credibilidade;
    ResultadoJogo resultado;

    // Debate: cursor da lista dupla circular e bitmap de quem já falou.
    NoListaDuplaCircular *turno_atual;
    unsigned int interrogados;
	int proximo_inicio_debate; // id do suspeito que deve abrir o próximo debate

    int ultima_vitima; // id do suspeito morto na última noite, ou NENHUM
    char revelacao_cumplice[256]; // mensagem revelada ao prender o cúmplice
} Jogo;

// ---------------------------------------------------------------------------
// Ciclo de vida
// ---------------------------------------------------------------------------
int iniciar_jogo(Jogo *jogo, int id_detetive);
int iniciar_jogo_com_semente(Jogo *jogo, int id_detetive, unsigned int semente);
static NoListaDuplaCircular *no_por_id(const Jogo *jogo, int id);
static NoListaDuplaCircular *primeiro_vivo_a_partir(const Jogo *jogo, NoListaDuplaCircular *comeco);
void liberar_jogo(Jogo *jogo);

// ---------------------------------------------------------------------------
// Fases
// ---------------------------------------------------------------------------
// Avança a fase e aplica os efeitos dela: pista na manhã, crime na noite,
// início do debate e derrota por tempo no fim do dia 4.
void avancar_fase_jogo(Jogo *jogo);

// ---------------------------------------------------------------------------
// Debate (uma volta completa pelos suspeitos vivos)
// ---------------------------------------------------------------------------
void iniciar_debate(Jogo *jogo);
Suspeito *suspeito_do_turno(const Jogo *jogo); // NULL quando a volta terminou
int perguntar_sobre(Jogo *jogo, int id_alvo);  // retorna o id do depoimento ou -1
int debate_encerrado(const Jogo *jogo);

// ---------------------------------------------------------------------------
// Julgamento
// ---------------------------------------------------------------------------
// Prende um suspeito vivo durante o julgamento. Retorna 1 se a ação foi aceita.
// Prender o assassino encerra o jogo em vitória; prender o cúmplice custa 15
// e revela uma informação; prender um inocente custa 30.
int prender_suspeito(Jogo *jogo, int id);

// ---------------------------------------------------------------------------
// Consultas usadas pela interface
// ---------------------------------------------------------------------------
Suspeito *buscar_suspeito(const Jogo *jogo, int id);
// Local verdadeiro do suspeito no dia atual (agenda). Retorna NULL se inválido.
const char *local_verdadeiro_do_suspeito(const Jogo *jogo, int id);
// Revelação guardada ao prender o cúmplice; string vazia se não houver.
const char *revelacao_do_cumplice(const Jogo *jogo);
int jogo_terminou(const Jogo *jogo);
const char *nome_fase(FaseDia fase);
const char *descricao_ultima_pista(const Jogo *jogo);
const char * nome_ultima_pista(const Jogo * jogo);

#endif
