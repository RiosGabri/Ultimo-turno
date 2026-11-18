#ifndef TELA_CADERNO_H
#define TELA_CADERNO_H

#include "interface/ui.h"
#include "entidades/caderno.h"

// Desenha a tela do caderno e processa suas interações.
// Retorna true quando o jogador solicita voltar à partida.
bool tela_caderno_atualizar_desenhar(
    const UIFonte *fonte,
    Caderno *caderno
);

// Inicia o monitoramento de contradições de uma nova partida.
void tela_caderno_iniciar_monitoramento(Caderno *caderno);

#endif