#include "listas/lista-dupla-circular.h"

#include <stdio.h>

typedef struct {
    int id;
    const char *nome;
} ItemTeste;

static int falhas = 0;

static void verificar(int condicao, const char *mensagem) {
    if (!condicao) {
        printf("FALHOU: %s\n", mensagem);
        falhas++;
    }
}

static void verificar_circular(const ListaDuplaCircular *lista, int quantidade_esperada) {
    if (lista == NULL) {
        verificar(0, "lista circular nao pode ser NULL");
        return;
    }

    if (quantidade_esperada == 0) {
        verificar(lista->inicio == NULL && lista->fim == NULL,
                  "lista vazia deve ter inicio e fim NULL");
        return;
    }

    if (lista->inicio == NULL || lista->fim == NULL) {
        verificar(0, "lista nao vazia deve ter inicio e fim");
        return;
    }

    int quantidade = 0;
    const NoListaDuplaCircular *atual = lista->inicio;

    do {
        verificar(atual->prox->ant == atual, "vinculo prox->ant incorreto");
        verificar(atual->ant->prox == atual, "vinculo ant->prox incorreto");
        quantidade++;

        if (quantidade > 100) {
            verificar(0, "loop circular invalido");
            return;
        }

        atual = atual->prox;
    } while (atual != lista->inicio);

    verificar(quantidade == quantidade_esperada, "quantidade de nos inesperada");
    verificar(lista->inicio->ant == lista->fim, "inicio->ant deve ser fim");
    verificar(lista->fim->prox == lista->inicio, "fim->prox deve ser inicio");
}

int main(void) {
    ListaDuplaCircular *lista = criar_lista_dupla_circular();

    if (lista == NULL) {
        printf("FALHOU: criar_lista_dupla_circular retornou NULL\n");
        return 1;
    }

    verificar_circular(lista, 0);
    verificar(lista_dupla_circular_buscar_por_id(lista, 1) == NULL,
              "busca em lista vazia");
    verificar(lista_dupla_circular_remover(lista, 1) == NULL,
              "remocao em lista vazia");

    ItemTeste a = {1, "um"};
    ItemTeste b = {2, "dois"};
    ItemTeste c = {3, "tres"};

    int inseriu = 1;
    inseriu &= lista_dupla_circular_inserir_no_fim(lista, a.id, &a);
    inseriu &= lista_dupla_circular_inserir_no_fim(lista, b.id, &b);
    inseriu &= lista_dupla_circular_inserir_no_fim(lista, c.id, &c);
    verificar(inseriu == 1, "inserir tres itens no fim");

    if (inseriu != 1) {
        liberar_lista_dupla_circular(lista);
        return 1;
    }

    verificar_circular(lista, 3);
    verificar(lista->inicio->id == 1, "inicio deve ser o primeiro item");
    verificar(lista->fim->id == 3, "fim deve ser o ultimo item");
    verificar(lista->inicio->ant == lista->fim, "inicio->ant deve ser fim");
    verificar(lista->fim->prox == lista->inicio, "fim->prox deve ser inicio");
    verificar(lista->inicio->prox->id == 2, "ordem 1 -> 2");
    verificar(lista->inicio->prox->ant == lista->inicio, "link 2 -> 1");
    verificar(lista->fim->ant->id == 2, "link 3 -> 2");
    verificar(lista->fim->ant->prox == lista->fim, "link 2 -> 3");
    verificar(lista_dupla_circular_buscar_por_id(lista, 1) == &a,
              "buscar primeiro item");
    verificar(lista_dupla_circular_buscar_por_id(lista, 2) == &b,
              "buscar item do meio");
    verificar(lista_dupla_circular_buscar_por_id(lista, 99) == NULL,
              "buscar item inexistente");

    lista_dupla_circular_listar(lista);

    // Contrato atual: troca apenas o dado; os ids dos nos permanecem.
    lista_dupla_circular_trocar_dados(lista, 1, 3);
    verificar(lista_dupla_circular_buscar_por_id(lista, 1) == &c,
              "troca deve mover o dado do id 1");
    verificar(lista_dupla_circular_buscar_por_id(lista, 3) == &a,
              "troca deve mover o dado do id 3");
    verificar(lista->inicio->id == 1 && lista->inicio->dado == &c,
              "id do no permanece apos troca");
    verificar(lista->fim->id == 3 && lista->fim->dado == &a,
              "id do ultimo no permanece apos troca");
    lista_dupla_circular_trocar_dados(lista, 1, 99);
    verificar(lista_dupla_circular_buscar_por_id(lista, 1) == &c,
              "troca com id inexistente nao altera o dado");
    verificar_circular(lista, 3);

    // Volta ao estado original para os testes de remocao.
    lista_dupla_circular_trocar_dados(lista, 1, 3);

    verificar(lista_dupla_circular_remover(lista, 2) == &b,
              "remover item do meio");
    verificar_circular(lista, 2);
    verificar(lista->inicio->id == 1 && lista->fim->id == 3,
              "remover meio deve manter as pontas");
    verificar(lista->inicio->prox == lista->fim,
              "remover meio deve religar os vizinhos");
    verificar(lista->fim->ant == lista->inicio,
              "remover meio deve religar o ant");
    verificar(lista->inicio->ant == lista->fim,
              "remover meio deve manter o circulo");
    verificar(lista->fim->prox == lista->inicio,
              "remover meio deve manter o circulo no fim");

    verificar(lista_dupla_circular_remover(lista, 1) == &a,
              "remover inicio");
    verificar_circular(lista, 1);
    verificar(lista->inicio == lista->fim && lista->fim->id == 3,
              "remover inicio deve atualizar o inicio");
    verificar(lista->inicio->prox == lista->inicio,
              "no unico deve apontar para si mesmo");
    verificar(lista->inicio->ant == lista->inicio,
              "no unico deve apontar para si mesmo no ant");

    verificar(lista_dupla_circular_remover(lista, 3) == &c,
              "remover unico item");
    verificar_circular(lista, 0);
    verificar(lista_dupla_circular_remover(lista, 99) == NULL,
              "remover item inexistente");

    inseriu = 1;
    inseriu &= lista_dupla_circular_inserir_no_fim(lista, a.id, &a);
    inseriu &= lista_dupla_circular_inserir_no_fim(lista, b.id, &b);
    inseriu &= lista_dupla_circular_inserir_no_fim(lista, c.id, &c);
    verificar(inseriu == 1, "reinserir tres itens");

    if (inseriu != 1) {
        liberar_lista_dupla_circular(lista);
        return 1;
    }

    verificar(lista_dupla_circular_remover(lista, 3) == &c,
              "remover ultimo item");
    verificar_circular(lista, 2);
    verificar(lista->fim->id == 2, "remover fim deve atualizar o ponteiro fim");
    verificar(lista->fim->prox == lista->inicio,
              "novo fim deve apontar para o inicio");
    verificar(lista->inicio->ant == lista->fim,
              "inicio deve apontar para o novo fim");

    verificar(lista_dupla_circular_remover(lista, 2) == &b,
              "remover segundo item");
    verificar_circular(lista, 1);
    verificar(lista->inicio->id == 1 && lista->fim->id == 1,
              "lista deve ficar com um unico no");
    verificar(lista_dupla_circular_remover(lista, 1) == &a,
              "remover ultimo no restante");
    verificar_circular(lista, 0);

    verificar(lista_dupla_circular_inserir_no_fim(lista, 4, NULL) == 0,
              "inserir dado NULL deve falhar");
    verificar(lista_dupla_circular_inserir_no_fim(NULL, 4, &a) == 0,
              "inserir em lista NULL deve falhar");
    verificar(lista_dupla_circular_buscar_por_id(NULL, 1) == NULL,
              "buscar em lista NULL");
    verificar(lista_dupla_circular_remover(NULL, 1) == NULL,
              "remover em lista NULL");
    lista_dupla_circular_listar(NULL);
    liberar_lista_dupla_circular(NULL);

    liberar_lista_dupla_circular(lista);

    if (falhas == 0) {
        printf("OK: todos os testes da lista circular passaram\n");
        return 0;
    }

    printf("FALHAS: %d\n", falhas);
    return 1;
}
