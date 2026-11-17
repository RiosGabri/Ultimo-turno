#include "raylib.h"

#include "telas.h"

#define TITULO_JOGO "Último Turno"

int main(void) {
    InitWindow(TELAS_LARGURA, TELAS_ALTURA, TITULO_JOGO);
    SetTargetFPS(60);

    telas_iniciar();

    while (!WindowShouldClose() && !telas_deve_fechar()) {
        telas_executar();
    }

    telas_liberar();
    CloseWindow();
    return 0;
}
