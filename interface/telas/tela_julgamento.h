
#ifndef TELA_JULGAMENTO_H
#define TELA_JULGAMENTO_H

#include "interface/recursos.h"
#include "interface/ui.h"
#include "jogo.h"

typedef enum {
    JULGAMENTO_ESCOLHENDO,
    JULGAMENTO_CONFIRMAR,
    JULGAMENTO_RESULTADO
} EtapaJulgamento;

typedef enum {
    JULGAMENTO_NENHUMA_ACAO,
    JULGAMENTO_CONTINUAR
} AcaoJulgamento;

typedef struct {
    Jogo *jogo;
    const Recursos *recursos;

    EtapaJulgamento etapa;

    // ID do suspeito escolhido ou NENHUM.
    int id_selecionado;

    // Credibilidade antes da decisão.
    int credibilidade_anterior;

    // Navegação por teclado e mouse.
    int botao_focado;

    BotaoUI botoes_suspeitos[TOTAL_SUSPEITOS];
    BotaoUI botao_nao_prender;
    BotaoUI botao_confirmar;
    BotaoUI botao_cancelar;
    BotaoUI botao_continuar;
} TelaJulgamento;

void tela_julgamento_iniciar(TelaJulgamento *tela, Jogo *jogo,
                            const Recursos *recursos);

AcaoJulgamento tela_julgamento_atualizar(TelaJulgamento *tela);

void tela_julgamento_desenhar(const TelaJulgamento *tela,
                              const UIFonte *fonte_titulo,
                              const UIFonte *fonte_corpo,
                              const UIFonte *fonte_corpo_bold);

#endif
