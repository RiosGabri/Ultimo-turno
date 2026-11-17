#ifndef TELA_CADERNO_H
#define TELA_CADERNO_H

#include "ui.h"
#include "entidades/caderno.h"

// Desenha a tela do caderno e processa suas interações.
// Retorna true quando o jogador solicita voltar à partida.
bool tela_caderno_atualizar_desenhar(
    const UIFonte *fonte,
    Caderno *caderno
);

#endif