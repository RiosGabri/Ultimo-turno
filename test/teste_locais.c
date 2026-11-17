#include "entidades/locais.h"

#include <stdio.h>
#include <string.h>

static int falhas = 0;

static void verificar(int condicao, const char *mensagem) {
    if (!condicao) {
        printf("FALHOU: %s\n", mensagem);
        falhas++;
    }
}

static void testar_tabela(void) {
    int total = total_locais();
    verificar(total > 0, "tabela tem pelo menos um local");

    for (int i = 0; i < total; i++) {
        const Local *local = local_por_indice(i);
        verificar(local != NULL, "índice válido retorna um local");
        if (local == NULL) {
            continue;
        }

        verificar(local->nome != NULL && local->nome[0] != '\0',
                  "local tem nome não vazio");
        verificar(local->preposicao != NULL && local->preposicao[0] != '\0',
                  "local tem preposição não vazia");
        verificar(strcmp(local->preposicao, "na") == 0 ||
                  strcmp(local->preposicao, "no") == 0,
                  "preposição é 'na' ou 'no'");

        const char *nome = nome_local(i);
        verificar(nome != NULL && strcmp(nome, local->nome) == 0,
                  "nome_local corresponde ao índice");

        const char *preposicao = preposicao_do_local(local->nome);
        verificar(preposicao != NULL &&
                  strcmp(preposicao, local->preposicao) == 0,
                  "preposicao_do_local encontra o nome cadastrado");
    }
}

static void testar_indices_invalidos(void) {
    int total = total_locais();

    verificar(local_por_indice(-1) == NULL, "índice negativo é recusado");
    verificar(local_por_indice(total) == NULL,
              "índice igual ao total é recusado");
    verificar(local_por_indice(total + 1) == NULL,
              "índice acima do total é recusado");

    verificar(nome_local(-1) == NULL, "nome de índice negativo é NULL");
    verificar(nome_local(total) == NULL, "nome de índice inválido é NULL");
}

static void testar_nomes_unicos(void) {
    int total = total_locais();
    int unicos = 1;

    for (int i = 0; i < total; i++) {
        const char *nome_i = nome_local(i);
        for (int j = i + 1; j < total; j++) {
            const char *nome_j = nome_local(j);
            if (nome_i != NULL && nome_j != NULL &&
                strcmp(nome_i, nome_j) == 0) {
                unicos = 0;
            }
        }
    }

    verificar(unicos, "não há nomes de locais repetidos");
}

static void testar_preposicao_desconhecida(void) {
    verificar(strcmp(preposicao_do_local(NULL), "em") == 0,
              "preposição de local NULL cai no padrão 'em'");
    verificar(strcmp(preposicao_do_local("Lugar inexistente"), "em") == 0,
              "preposição de local desconhecido cai no padrão 'em'");
}

int main(void) {
    testar_tabela();
    testar_indices_invalidos();
    testar_nomes_unicos();
    testar_preposicao_desconhecida();

    if (falhas == 0) {
        printf("OK: todos os testes de locais passaram\n");
        return 0;
    }

    printf("FALHAS: %d\n", falhas);
    return 1;
}
