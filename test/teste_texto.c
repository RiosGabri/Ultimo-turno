#include "entidades/texto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int falhas = 0;

static void verificar(int condicao, const char *mensagem) {
    if (!condicao) {
        printf("FALHOU: %s\n", mensagem);
        falhas++;
    }
}

static void testar_copia_simples(void) {
    const char *original = "Último Turno — ação";
    char *copia = copiar_texto(original);

    verificar(copia != NULL, "copiar_texto deve alocar a cópia");
    if (copia != NULL) {
        verificar(strcmp(copia, original) == 0, "a cópia tem o mesmo conteúdo");
        verificar(copia != original, "a cópia não aponta para a string original");
        free(copia);
    }
}

static void testar_independencia(void) {
    char original[32] = "Temporário";
    char *copia = copiar_texto(original);

    verificar(copia != NULL, "copiar_texto deve alocar a cópia");
    if (copia == NULL) {
        return;
    }

    original[0] = 'X';
    verificar(strcmp(copia, "Temporário") == 0,
              "alterar a origem não altera a cópia");
    free(copia);
}

static void testar_entradas_invalidas(void) {
    verificar(copiar_texto(NULL) == NULL, "copiar_texto(NULL) retorna NULL");
}

int main(void) {
    testar_copia_simples();
    testar_independencia();
    testar_entradas_invalidas();

    if (falhas == 0) {
        printf("OK: todos os testes de texto passaram\n");
        return 0;
    }

    printf("FALHAS: %d\n", falhas);
    return 1;
}
