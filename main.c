#include "raylib.h"

#include "telas.h"

#define TITULO_JOGO "Último Turno"

int main(void) {
    // A janela pode ser redimensionada ou alternada para tela cheia; o jogo
    // desenha num canvas virtual e escala o conjunto (texto incluso).
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(TELAS_LARGURA, TELAS_ALTURA, TITULO_JOGO);
    SetWindowMinSize(960, 540);
    SetTargetFPS(60);

    telas_iniciar();

    while (!WindowShouldClose() && !telas_deve_fechar()) {
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }
        telas_executar();
    }

    telas_liberar();
    CloseWindow();
    return 0;
}
