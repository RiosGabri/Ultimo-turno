#include "listas/lista-dupla.h"

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

static void verificar_dupla(const ListaDupla *lista, int quantidade_esperada) {
    if (lista == NULL) {
        verificar(0, "lista dupla nao pode ser NULL");
        return;
    }

    int quantidade = 0;
    const NoListaDupla *anterior = NULL;
    const NoListaDupla *atual = lista->inicio;

    while (atual != NULL) {
        verificar(atual->ant == anterior, "vinculo ant incorreto");
        anterior = atual;
        atual = atual->prox;
        quantidade++;
    }

    verificar(anterior == lista->fim, "fim deve apontar para o ultimo no");
    verificar(lista->fim == NULL || lista->fim->prox == NULL,
              "fim->prox deve ser NULL");
    verificar(quantidade == quantidade_esperada, "quantidade de nos inesperada");
}

int main(void) {
    ListaDupla *lista = criar_lista_dupla();

    if (lista == NULL) {
        printf("FALHOU: criar_lista_dupla retornou NULL\n");
        return 1;
    }

    verificar_dupla(lista, 0);
    verificar(lista_dupla_buscar_por_id(lista, 1) == NULL, "busca em lista vazia");
    verificar(lista_dupla_remover(lista, 1) == NULL, "remocao em lista vazia");

    ItemTeste a = {1, "um"};
    ItemTeste b = {2, "dois"};
    ItemTeste c = {3, "tres"};

    int inseriu = 1;
    inseriu &= lista_dupla_inserir_no_fim(lista, a.id, &a);
    inseriu &= lista_dupla_inserir_no_fim(lista, b.id, &b);
    inseriu &= lista_dupla_inserir_no_fim(lista, c.id, &c);
    verificar(inseriu == 1, "inserir tres itens no fim");

    if (inseriu != 1) {
        liberar_lista_dupla(lista);
        return 1;
    }

    verificar_dupla(lista, 3);
    verificar(lista->inicio->id == 1, "inicio deve ser o primeiro item");
    verificar(lista->fim->id == 3, "fim deve ser o ultimo item");
    verificar(lista->inicio->ant == NULL, "inicio->ant deve ser NULL");
    verificar(lista->inicio->prox->id == 2, "ordem 1 -> 2");
    verificar(lista->inicio->prox->ant == lista->inicio, "link 2 -> 1");
    verificar(lista->fim->ant->id == 2, "link 3 -> 2");
    verificar(lista->fim->prox == NULL, "fim->prox deve ser NULL");
    verificar(lista_dupla_buscar_por_id(lista, 1) == &a, "buscar primeiro item");
    verificar(lista_dupla_buscar_por_id(lista, 2) == &b, "buscar item do meio");
    verificar(lista_dupla_buscar_por_id(lista, 99) == NULL, "buscar item inexistente");

    lista_dupla_listar(lista);

    // Contrato atual: troca apenas o dado; os ids dos nos permanecem.
    lista_dupla_trocar_dados(lista, 1, 3);
    verificar(lista_dupla_buscar_por_id(lista, 1) == &c,
              "troca deve mover o dado do id 1");
    verificar(lista_dupla_buscar_por_id(lista, 3) == &a,
              "troca deve mover o dado do id 3");
    verificar(lista->inicio->id == 1 && lista->inicio->dado == &c,
              "id do no permanece apos troca");
    verificar(lista->fim->id == 3 && lista->fim->dado == &a,
              "id do ultimo no permanece apos troca");
    lista_dupla_trocar_dados(lista, 1, 99);
    verificar(lista_dupla_buscar_por_id(lista, 1) == &c,
              "troca com id inexistente nao altera o dado");
    verificar_dupla(lista, 3);

    // Volta ao estado original para os testes de remocao.
    lista_dupla_trocar_dados(lista, 1, 3);

    verificar(lista_dupla_remover(lista, 2) == &b, "remover item do meio");
    verificar_dupla(lista, 2);
    verificar(lista->inicio->id == 1 && lista->fim->id == 3,
              "remover meio deve manter as pontas");
    verificar(lista->inicio->prox == lista->fim,
              "remover meio deve religar os vizinhos");
    verificar(lista->fim->ant == lista->inicio,
              "remover meio deve religar o ant");
    verificar(lista->fim->prox == NULL, "remover meio nao deve quebrar o fim");

    verificar(lista_dupla_remover(lista, 1) == &a, "remover inicio");
    verificar_dupla(lista, 1);
    verificar(lista->inicio == lista->fim && lista->fim->id == 3,
              "remover inicio deve atualizar o inicio");
    verificar(lista->inicio->ant == NULL && lista->inicio->prox == NULL,
              "no unico deve ter ant/prox NULL");

    verificar(lista_dupla_remover(lista, 3) == &c, "remover unico item");
    verificar_dupla(lista, 0);
    verificar(lista_dupla_remover(lista, 99) == NULL, "remover item inexistente");

    inseriu = 1;
    inseriu &= lista_dupla_inserir_no_fim(lista, a.id, &a);
    inseriu &= lista_dupla_inserir_no_fim(lista, b.id, &b);
    inseriu &= lista_dupla_inserir_no_fim(lista, c.id, &c);
    verificar(inseriu == 1, "reinserir tres itens");

    if (inseriu != 1) {
        liberar_lista_dupla(lista);
        return 1;
    }

    verificar(lista_dupla_remover(lista, 3) == &c, "remover ultimo item");
    verificar_dupla(lista, 2);
    verificar(lista->fim->id == 2 && lista->fim->prox == NULL,
              "remover fim deve atualizar o ponteiro fim");
    verificar(lista->fim->ant == lista->inicio,
              "novo fim deve apontar para o anterior");

    verificar(lista_dupla_remover(lista, 2) == &b, "remover segundo item");
    verificar(lista_dupla_remover(lista, 1) == &a, "remover primeiro item novamente");
    verificar_dupla(lista, 0);

    verificar(lista_dupla_inserir_no_fim(lista, 4, NULL) == 0,
              "inserir dado NULL deve falhar");
    verificar(lista_dupla_inserir_no_fim(NULL, 4, &a) == 0,
              "inserir em lista NULL deve falhar");
    verificar(lista_dupla_buscar_por_id(NULL, 1) == NULL, "buscar em lista NULL");
    verificar(lista_dupla_remover(NULL, 1) == NULL, "remover em lista NULL");
    liberar_lista_dupla(NULL);

    liberar_lista_dupla(lista);

    if (falhas == 0) {
        printf("OK: todos os testes da lista dupla passaram\n");
        return 0;
    }

    printf("FALHAS: %d\n", falhas);
    return 1;
}
