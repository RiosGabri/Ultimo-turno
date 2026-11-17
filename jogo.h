#ifndef JOGO_H
#define JOGO_H

#include "dia.h"
#include "entidades/aleatorio.h"
#include "entidades/caderno.h"
#include "entidades/personagens.h"
#include "entidades/suspeito.h"
#include "listas/lista-dupla-circular.h"

#define CREDIBILIDADE_INICIAL 100
#define CUSTO_PRENDER_INOCENTE 25
#define CUSTO_PRENDER_CUMPLICE 15

// Pesos da pontuação da partida.
#define PONTOS_VITORIA 3000           // bônus por fechar o caso
#define PONTOS_POR_DIA_RESTANTE 200   // cada dia ainda não usado
#define PONTOS_POR_CREDIBILIDADE 10   // cada ponto de credibilidade restante
#define PONTOS_POR_CONTRADICAO 50     // cada contradição registrada no caderno

typedef enum {
    JOGO_EM_ANDAMENTO,
    JOGO_VITORIA,
    JOGO_DERROTA_CREDIBILIDADE,
    JOGO_DERROTA_TEMPO
} ResultadoJogo;

#define TOTAL_PERIODOS 3
typedef struct {
    //[dia][período][suspeito] -> índice na tabela de locais
    //unsigned char armazena valores nao negativos de 0 a 255
    unsigned char local[TOTAL_DIAS][TOTAL_PERIODOS][TOTAL_SUSPEITOS];
    unsigned char local_crime[TOTAL_DIAS]; // índice na tabela de locais
}Agenda;

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
    Agenda agenda;
    int credibilidade;
    ResultadoJogo resultado;

    // Debate: cursor da lista dupla circular e bitmap de quem já falou.
    NoListaDuplaCircular *turno_atual;
    unsigned int interrogados;

    int ultima_vitima; // id do suspeito morto na última noite, ou NENHUM
    char revelacao_cumplice[256]; // mensagem revelada ao prender o cúmplice

    // Gerador compartilhado dos sorteios da partida (papéis, agenda e vítima
    // noturna). Usa xorshift32, então a mesma semente dá o mesmo jogo em
    // qualquer plataforma, independente de rand().
    Aleatorio aleatorio;
} Jogo;

// ---------------------------------------------------------------------------
// Ciclo de vida
// ---------------------------------------------------------------------------
int iniciar_jogo(Jogo *jogo, int id_detetive);
int iniciar_jogo_com_semente(Jogo *jogo, int id_detetive, unsigned int semente);
void liberar_jogo(Jogo *jogo);

const char *local_do_suspeito(const Jogo*, int id, int dia, enum Periodo);
const char *local_do_crime(const Jogo*, int dia);

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
int perguntar_sobre(Jogo *jogo, int id_alvo, enum Periodo periodo);  // retorna o id do depoimento ou -1
int debate_encerrado(const Jogo *jogo);

// ---------------------------------------------------------------------------
// Julgamento
// ---------------------------------------------------------------------------
// Prende um suspeito vivo durante o julgamento. Retorna 1 se a ação foi aceita.
// Prender o assassino encerra o jogo em vitória; prender o cúmplice custa 15
// e revela uma informação; prender um inocente custa 25.
int prender_suspeito(Jogo *jogo, int id);

// ---------------------------------------------------------------------------
// Pontuação e final
// ---------------------------------------------------------------------------
// Pontuação pelos três critérios da partida: dias usados, credibilidade
// restante e contradições encontradas. Fora da faixa válida, cada valor é
// limitado antes do cálculo (o bônus de vitória entra em pontuacao_do_jogo).
int calcular_pontuacao(int dias_usados, int credibilidade_restante,
                       int contradicoes_encontradas);

// Pontuação da partida atual: o cálculo acima mais PONTOS_VITORIA quando o
// jogo terminou em vitória. Retorna 0 para jogo NULL.
int pontuacao_do_jogo(const Jogo *jogo);

// Texto narrativo do final: vitória, derrota por credibilidade ou derrota por
// tempo. String vazia se o jogo ainda está em andamento ou se for inválido.
const char *texto_final(const Jogo *jogo);

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
