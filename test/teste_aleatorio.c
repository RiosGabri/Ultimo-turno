#include "entidades/aleatorio.h"

#include <stdio.h>

static int falhas = 0;

static void verificar(int condicao, const char *mensagem) {
    if (!condicao) {
        printf("FALHOU: %s\n", mensagem);
        falhas++;
    }
}

static void testar_mesma_semente_mesma_sequencia(void) {
    Aleatorio primeiro;
    Aleatorio segundo;
    aleatorio_iniciar(&primeiro, 42);
    aleatorio_iniciar(&segundo, 42);

    for (int i = 0; i < 32; i++) {
        uint32_t a = aleatorio_proximo(&primeiro);
        uint32_t b = aleatorio_proximo(&segundo);
        if (a != b) {
            verificar(0, "a mesma semente gera a mesma sequência");
            return;
        }
    }

    verificar(1, "a mesma semente gera a mesma sequência");
}

static void testar_sementes_diferentes(void) {
    Aleatorio um;
    Aleatorio outro;
    aleatorio_iniciar(&um, 1);
    aleatorio_iniciar(&outro, 2);

    int diferentes = 0;
    for (int i = 0; i < 8; i++) {
        if (aleatorio_proximo(&um) != aleatorio_proximo(&outro)) {
            diferentes = 1;
            break;
        }
    }

    verificar(diferentes, "sementes diferentes geram sequências diferentes");
}

static void testar_reinicio(void) {
    Aleatorio gerador;
    aleatorio_iniciar(&gerador, 2026);

    uint32_t antes[8];
    for (int i = 0; i < 8; i++) {
        antes[i] = aleatorio_proximo(&gerador);
    }

    aleatorio_iniciar(&gerador, 2026);

    int igual = 1;
    for (int i = 0; i < 8; i++) {
        if (antes[i] != aleatorio_proximo(&gerador)) {
            igual = 0;
            break;
        }
    }

    verificar(igual, "reiniciar com a mesma semente repete a sequência");
}

static void testar_semente_zero(void) {
    Aleatorio gerador;
    aleatorio_iniciar(&gerador, 0);

    int todos_zero = 1;
    int todos_iguais = 1;
    uint32_t primeiro = 0;

    for (int i = 0; i < 16; i++) {
        uint32_t valor = aleatorio_proximo(&gerador);
        if (valor != 0) {
            todos_zero = 0;
        }
        if (i == 0) {
            primeiro = valor;
        } else if (valor != primeiro) {
            todos_iguais = 0;
        }
        verificar(gerador.estado != 0, "o estado nunca fica preso em zero");
    }

    verificar(!todos_zero, "a semente zero não trava o gerador em zero");
    verificar(!todos_iguais, "a semente zero gera valores variados");
}

static void testar_intervalo(void) {
    Aleatorio gerador;
    aleatorio_iniciar(&gerador, 7);

    const int limite = 6;
    int vistos[6] = {0, 0, 0, 0, 0, 0};
    int fora = 0;

    for (int i = 0; i < 600; i++) {
        int valor = aleatorio_intervalo(&gerador, limite);
        if (valor < 0 || valor >= limite) {
            fora++;
        } else {
            vistos[valor] = 1;
        }
    }

    verificar(fora == 0, "o intervalo respeita [0, limite - 1]");

    int todos_vistos = 1;
    for (int i = 0; i < limite; i++) {
        if (!vistos[i]) {
            todos_vistos = 0;
        }
    }
    verificar(todos_vistos, "o intervalo cobre todos os valores possíveis");
}

static void testar_limites_invalidos(void) {
    Aleatorio gerador;
    aleatorio_iniciar(&gerador, 3);

    verificar(aleatorio_intervalo(&gerador, 0) == 0,
              "limite zero retorna 0");
    verificar(aleatorio_intervalo(&gerador, -5) == 0,
              "limite negativo retorna 0");
    verificar(aleatorio_intervalo(NULL, 6) == 0,
              "gerador NULL retorna 0");
    verificar(aleatorio_proximo(NULL) == 0,
              "próximo com gerador NULL retorna 0");

    aleatorio_iniciar(NULL, 1); // não deve quebrar
    verificar(1, "iniciar com gerador NULL não quebra");
}

int main(void) {
    testar_mesma_semente_mesma_sequencia();
    testar_sementes_diferentes();
    testar_reinicio();
    testar_semente_zero();
    testar_intervalo();
    testar_limites_invalidos();

    if (falhas == 0) {
        printf("OK: todos os testes de aleatoriedade passaram\n");
        return 0;
    }

    printf("FALHAS: %d\n", falhas);
    return 1;
}
