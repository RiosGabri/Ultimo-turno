#ifndef TELAS_H
#define TELAS_H

#include <stdbool.h>

#include "jogo.h"
#include "raylib.h"
#include "recursos.h"
#include "ui.h"

#define TELAS_LARGURA 1280
#define TELAS_ALTURA 720

// Inicializa fontes, recursos e o estado da interface.
void telas_iniciar(void);

// Executa um frame: lê entrada, atualiza o estado e desenha.
void telas_executar(void);

void telas_liberar(void);

bool telas_deve_fechar(void);

#endif
