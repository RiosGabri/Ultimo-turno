#ifndef TELA_DEBATE_H
#define TELA_DEBATE_H
#include "interface/ui.h"
#include "jogo.h"

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

    EtapaDebate etapa;
    int id_alvo;
    int id_depoimento;

    // Controle do aviso de nova contradição.
    int contradicoes_antes;
    float tempo_alerta;

    // Navegação por teclado/mouse.
    int botao_focado;
    bool interrogar_pressionado;

    // Estado visual dos botões (BotaoUI).
    BotaoUI botao_interrogar;
    BotaoUI botao_caderno;
    BotaoUI botao_encerrar;
    BotaoUI botoes_alvo[TOTAL_SUSPEITOS];
    BotaoUI botoes_periodo[TOTAL_PERIODOS];

    // Assets provisórios (fundo e retratos). id == 0 means unavailable.
    Texture2D fundo;
    Texture2D retratos[TOTAL_SUSPEITOS];
} TelaDebate;

void tela_debate_iniciar(TelaDebate *tela, Jogo *jogo);
void tela_debate_liberar(TelaDebate *tela);
AcaoDebate tela_debate_atualizar(TelaDebate *tela);
void tela_debate_desenhar(const TelaDebate *tela, const UIFonte *fonte);
#endif