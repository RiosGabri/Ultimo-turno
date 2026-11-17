#ifndef TELA_DEBATE_H
#define TELA_DEBATE_H

#include <stdbool.h>

#include "interface/recursos.h"
#include "interface/ui.h"
#include "jogo.h"

/*
 * Tela do debate: escolha do alvo, período, depoimento, reações e aviso de
 * nova contradição. O estado transitório fica aqui; o motor continua em Jogo.
 */

typedef enum {
    DEBATE_ESCOLHENDO_ALVO,
    DEBATE_ESCOLHENDO_PERIODO,
    DEBATE_MOSTRANDO_DEPOIMENTO
} EtapaDebate;

typedef enum {
    TELA_DEBATE_CONTINUAR,
    TELA_DEBATE_ABRIR_CADERNO,
    TELA_DEBATE_ENCERRAR
} AcaoDebate;

typedef struct {
    Jogo *jogo;
    const Recursos *recursos;

    EtapaDebate etapa;
    int id_alvo;
    int id_depoimento;

    // Aviso de nova contradição: tempo restante em segundos.
    float tempo_alerta;

    // Navegação por teclado/mouse.
    int botao_focado;

    // Estado visual dos botões (BotaoUI).
    BotaoUI botao_interrogar;
    BotaoUI botao_caderno;
    BotaoUI botao_encerrar;
    BotaoUI botao_voltar_periodo;
    BotaoUI botoes_alvo[TOTAL_SUSPEITOS];
    BotaoUI botoes_periodo[TOTAL_PERIODOS];
} TelaDebate;

// `recursos` é emprestado e deve continuar vivo enquanto a tela existir.
void tela_debate_iniciar(TelaDebate *tela, Jogo *jogo,
                         const Recursos *recursos);

AcaoDebate tela_debate_atualizar(TelaDebate *tela);

// Os textos usam três fontes: títulos (Playfair), corpo e etiquetas (Courier).
void tela_debate_desenhar(const TelaDebate *tela,
                          const UIFonte *fonte_titulo,
                          const UIFonte *fonte_corpo,
                          const UIFonte *fonte_corpo_bold);

#endif
