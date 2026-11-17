#include "caderno.h"
#include "entidades/texto.h"
#include "listas/lista-dupla.h"
#include "listas/lista-simples.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Caderno* inicializar_caderno(int id) {
    Caderno *caderno = malloc(sizeof(Caderno));
    if (caderno == NULL) {
        return NULL; // Falha ao alocar memória
    }

    caderno->id = id;
    //inicializar as seções do caderno todas vazias
    caderno->depoimentos.inicio = NULL;
    caderno->depoimentos.fim = NULL;
    caderno->depoimentos.atual = NULL;
    caderno->depoimentos.tamanho = 0;

    caderno->pistas.inicio = NULL;
    caderno->pistas.fim = NULL;
    caderno->pistas.atual = NULL;
    caderno->pistas.tamanho = 0;

    caderno->reacoes.inicio = NULL;
    caderno->reacoes.fim = NULL;
    caderno->reacoes.atual = NULL;
    caderno->reacoes.tamanho = 0;

    caderno->contradicoes.inicio = NULL;
    caderno->contradicoes.fim = NULL;
    caderno->contradicoes.atual = NULL;
    caderno->contradicoes.tamanho = 0;

    caderno->prox_id_depoimento = 1; 
    caderno->prox_id_pista = 1;
    caderno->prox_id_reacao = 1;
    caderno->secao_ativa = SECAO_DEPOIMENTOS;

    return caderno;
}

int registrar_depoimento(Caderno *caderno, int dia, int quem_falou, char* local, enum Periodo periodo, int sobre_quem_fala, enum Categorias categoria, char* texto) {
    //nó do novo depoimento que fica na seção de depoimentos do caderno
    NoListaDupla *novo_depoimento = malloc(sizeof(NoListaDupla));
    if (novo_depoimento == NULL) {
        return -1; // Falha ao alocar memória
    }
    
    //depoimento em si, que será armazenado no nó da lista dupla
    Depoimento *depoimento = malloc(sizeof(Depoimento));
    if (depoimento == NULL) {
        free(novo_depoimento);
        return -1; // Falha ao alocar memória
    }
    depoimento->dia = dia;
    depoimento->id_falante = quem_falou;
    depoimento->local = copiar_texto(local);
    depoimento->periodo = periodo;
    depoimento->id_assunto = sobre_quem_fala;
    depoimento->texto = copiar_texto(texto);
    depoimento->categoria = categoria;

    if (depoimento->local == NULL || depoimento->texto == NULL) {
        free(depoimento->local);
        free(depoimento->texto);
        free(depoimento);
        free(novo_depoimento);
        return -1; // Falha ao copiar os textos
    }

    depoimento->id_depoimento = caderno->prox_id_depoimento++; // Atribui o próximo id de depoimento e incrementa

    //adicionando o depoimento na seção de depoimentos do caderno
    novo_depoimento->id = depoimento->id_depoimento; 
    novo_depoimento->dado = depoimento;
    novo_depoimento->prox = NULL;
    novo_depoimento->ant = NULL;

    if (caderno->depoimentos.fim == NULL) {
        caderno->depoimentos.inicio = novo_depoimento;
    } else {
        caderno->depoimentos.fim->prox = novo_depoimento;
        novo_depoimento->ant = caderno->depoimentos.fim;
    }
    caderno->depoimentos.fim = novo_depoimento;
    caderno->depoimentos.tamanho++;

    verificar_conflitos_do_depoimento(caderno, depoimento);

    return depoimento->id_depoimento; // Retorna o ID do depoimento registrado
}

int verificar_conflitos_do_depoimento(Caderno * caderno, Depoimento * novo){
    if(caderno == NULL || novo == NULL){
        return 0;
    }

    int total_contradicoes = 0;

    for(NoListaDupla * atual = caderno->depoimentos.inicio; atual != NULL; atual = atual->prox){
        Depoimento * outro = atual->dado;

        if(outro->id_depoimento == novo->id_depoimento){
            continue;
        }
        if(!depoimentos_comparaveis(novo, outro)){
            continue;
        }
        if(depoimentos_sao_contraditorios(novo, outro)){
            if(registrar_contradicao(caderno, novo, outro)){
                total_contradicoes ++;
            }
        }
    }

    return total_contradicoes;
}

void registrar_pista(Caderno *caderno, int dia, char* local, char* objeto, char* descricao, ListaDupla *suspeitos_relacionados) {
    //nó do novo depoimento que fica na seção de depoimentos do caderno
    NoListaDupla *nova_pista = malloc(sizeof(NoListaDupla));
    if (nova_pista == NULL) {
        liberar_lista_dupla(suspeitos_relacionados);
        return; // Falha ao alocar memória
    }

    //pista em si, que será armazenado no nó da lista dupla
    Pista *pista = malloc(sizeof(Pista));
    if (pista == NULL) {
        free(nova_pista);
        liberar_lista_dupla(suspeitos_relacionados);
        return; // Falha ao alocar memória
    }
    pista->dia = dia;
    pista->local = copiar_texto(local);
    pista->objeto = copiar_texto(objeto);
    pista->descricao = copiar_texto(descricao);
    pista->suspeitos_relacionados = suspeitos_relacionados;

    if (pista->local == NULL || pista->objeto == NULL || pista->descricao == NULL) {
        free(pista->local);
        free(pista->objeto);
        free(pista->descricao);
        free(pista);
        free(nova_pista);
        liberar_lista_dupla(suspeitos_relacionados);
        return; // Falha ao copiar os textos
    }

    //adicionando o depoimento na seção de depoimentos do caderno
    nova_pista->id = caderno->prox_id_pista++; // Atribui um ID único para a pista
    nova_pista->dado = pista;
    nova_pista->prox = NULL;
    nova_pista->ant = NULL;

    if (caderno->pistas.fim == NULL) {
        caderno->pistas.inicio = nova_pista;
    } else {
        caderno->pistas.fim->prox = nova_pista;
        nova_pista->ant = caderno->pistas.fim;
    }
    caderno->pistas.fim = nova_pista;
    caderno->pistas.tamanho++;
}
int pista_tem_suspeito_relacionado(const Pista *pista, int id_suspeito) {
    if (pista == NULL || pista->suspeitos_relacionados == NULL) {
        return 0;
    }
    for (const NoListaDupla *no = pista->suspeitos_relacionados->inicio; no != NULL; no = no->prox) {
        if (no->id == id_suspeito) {
            return 1;
        }
    }
    return 0;
}

void registrar_reacao(Caderno *caderno, int dia, int id_depoimento, char* quem_reagiu, enum TipoReacao tipo_reacao) {
    //nó da nova reação que fica na seção de reações do caderno
    NoListaDupla *nova_reacao = malloc(sizeof(NoListaDupla));
    if (nova_reacao == NULL) {
        return; // Falha ao alocar memória
    }

    //reação em si, que será armazenada no nó da lista dupla
    Reacao *reacao = malloc(sizeof(Reacao));
    if (reacao == NULL) {
        free(nova_reacao);
        return; // Falha ao alocar memória
    }
    reacao->dia = dia;
    reacao->id_depoimento = id_depoimento;
    reacao->quem_reagiu = copiar_texto(quem_reagiu);
    reacao->tipo_reacao = tipo_reacao;

    if (reacao->quem_reagiu == NULL) {
        free(reacao);
        free(nova_reacao);
        return; // Falha ao copiar o texto
    }

    //adicionando a reação na seção de reações do caderno
    nova_reacao->id = caderno->prox_id_reacao++; // Atribui um ID único para a reação
    nova_reacao->dado = reacao;
    nova_reacao->prox = NULL;
    nova_reacao->ant = NULL;

    if (caderno->reacoes.fim == NULL) {
        caderno->reacoes.inicio = nova_reacao;
    } else {
        caderno->reacoes.fim->prox = nova_reacao;
        nova_reacao->ant = caderno->reacoes.fim;
    }
    caderno->reacoes.fim = nova_reacao;
    caderno->reacoes.tamanho++;
}

static Secao *obter_secao(Caderno *caderno, SecaoCaderno secao) {
    switch (secao) {
        case SECAO_DEPOIMENTOS: return &caderno->depoimentos;
        case SECAO_PISTAS:      return &caderno->pistas;
        case SECAO_REACOES:     return &caderno->reacoes;
        case SECAO_CONTRADICOES:return &caderno->contradicoes;
        default:                return NULL;
    }
}

void selecionar_secao(Caderno *caderno, SecaoCaderno secao) {
    if (caderno == NULL || secao < 0 || secao >= TOTAL_SECOES) {
        return;
    }

    caderno->secao_ativa = secao;

    // Ao entrar numa seção, começa pelo primeiro item. Se já havia um item
    // selecionado (ex.: voltou para a seção), mantém a posição.
    Secao *ativa = obter_secao(caderno, secao);
    if (ativa != NULL && ativa->atual == NULL) {
        ativa->atual = ativa->inicio;
    }
}

void avancar_secao(Caderno *caderno){
    if (caderno == NULL) {
        return;
    }

    Secao *ativa = obter_secao(caderno, caderno->secao_ativa);
    if (ativa == NULL) {
        return;
    }

    if (ativa->atual == NULL) {
        ativa->atual = ativa->inicio;
    } else if (ativa->atual->prox != NULL) {
        ativa->atual = ativa->atual->prox;
    }
}

void voltar_secao(Caderno *caderno){
    if (caderno == NULL) {
        return;
    }

    Secao *ativa = obter_secao(caderno, caderno->secao_ativa);
    if (ativa == NULL) {
        return;
    }

    if (ativa->atual == NULL) {
        ativa->atual = ativa->inicio;
    } else if (ativa->atual->ant != NULL) {
        ativa->atual = ativa->atual->ant;
    }
}

NoListaDupla *obter_item_atual(Caderno *caderno){
    if (caderno == NULL) {
        return NULL;
    }

    Secao *ativa = obter_secao(caderno, caderno->secao_ativa);
    return (ativa != NULL) ? ativa->atual : NULL;
}

void ler_item_atual(Caderno *caderno){
    NoListaDupla *atual = obter_item_atual(caderno);
    if (atual == NULL) {
        printf("Nenhum item atual para ler.\n");
        return;
    }

    switch (caderno->secao_ativa) {
        case SECAO_DEPOIMENTOS: {
            Depoimento *depoimento = (Depoimento *)atual->dado;
            printf("Depoimento do suspeito %d: %s\n", depoimento->id_falante, depoimento->texto);
            break;
        }
        case SECAO_PISTAS: {
            Pista *pista = (Pista *)atual->dado;
            printf("Pista encontrada no dia %d: %s\n", pista->dia, pista->descricao);
            break;
        }
        case SECAO_REACOES: {
            Reacao *reacao = (Reacao *)atual->dado;
            printf("Reação de %s ao depoimento %d: %d\n", reacao->quem_reagiu, reacao->id_depoimento, reacao->tipo_reacao);
            break;
        }
        case SECAO_CONTRADICOES: {
            Contradicao *contradicao = (Contradicao *)atual->dado;
            printf("Contradição entre os depoimentos %d e %d\n",
                   contradicao->depoimento1->id_depoimento,
                   contradicao->depoimento2->id_depoimento);
            break;
        }
        default:
            printf("Nenhum item atual para ler.\n");
            break;
    }
}

static int comparar_pistas(const Pista *a, const Pista *b,
                          CriterioOrdenacao criterio) {
    switch (criterio) {
        case ORDENAR_POR_LOCAL:
            return strcmp(a->local, b->local);

        case ORDENAR_POR_OBJETO:
            return strcmp(a->objeto, b->objeto);

        case ORDENAR_POR_DIA_RECENTE:
            return b->dia - a->dia;

        default:
            return 0;
    }
}

// Troca o conteúdo dos nós (id + dado) para que o id do nó continue
// apontando para a mesma pista depois da ordenação.
static void trocar_conteudo_nos(NoListaDupla *a, NoListaDupla *b) {
    int id = a->id;
    void *dado = a->dado;
    a->id = b->id;
    a->dado = b->dado;
    b->id = id;
    b->dado = dado;
}

int ordenar_pistas(Caderno *caderno, CriterioOrdenacao criterio) {
    if (caderno == NULL || caderno->pistas.tamanho < 2) {
        return 0;
    }

    int quantidade = caderno->pistas.tamanho;
    int total_trocas = 0;

    for (int i = 0; i < quantidade - 1; i++) {
        int trocou = 0;
        NoListaDupla *atual = caderno->pistas.inicio;

        for (int j = 0; j < quantidade - 1 - i; j++) {
            Pista *pista_atual = (Pista *)atual->dado;
            Pista *pista_proxima = (Pista *)atual->prox->dado;

            if (comparar_pistas(pista_atual, pista_proxima, criterio) > 0) {
                trocar_conteudo_nos(atual, atual->prox);
                total_trocas++;
                trocou = 1;
            }

            atual = atual->prox;
        }

        if (!trocou) {
            break;
        }
    }

    return total_trocas;
}

void ordenar_pistas_por_dia(Caderno *caderno) {
    if (caderno == NULL || caderno->pistas.tamanho < 2) {
        return;
    }

    int quantidade = caderno->pistas.tamanho;

    // Bubble sort: a cada passada o maior dia "afunda" para o fim.
    // A comparação estrita (> ) mantém a ordenação estável.
    for (int i = 0; i < quantidade - 1; i++) {
        int trocou = 0;
        NoListaDupla *atual = caderno->pistas.inicio;

        for (int j = 0; j < quantidade - 1 - i && atual != NULL && atual->prox != NULL; j++) {
            Pista *pista_atual = (Pista *)atual->dado;
            Pista *pista_proxima = (Pista *)atual->prox->dado;

            if (pista_atual != NULL && pista_proxima != NULL &&
                pista_atual->dia > pista_proxima->dia) {
                trocar_conteudo_nos(atual, atual->prox);
                trocou = 1;
            }
            atual = atual->prox;
        }

        if (!trocou) {
            break; // lista já ordenada
        }
    }
}

Depoimento* buscar_depoimento_por_id(Caderno *caderno, int id_depoimento) {
    if (caderno == NULL) {
        return NULL; // Caderno nulo, não há depoimentos
    }
    NoListaDupla *atual = caderno->depoimentos.inicio;
    while (atual != NULL) {
        Depoimento *depoimento = (Depoimento *)atual->dado;
        if (depoimento->id_depoimento == id_depoimento) {
            return depoimento; // Depoimento encontrado
            }
            atual = atual->prox;
        }
    return NULL; // Depoimento não encontrado
}

void liberar_caderno(Caderno *caderno) {
    // Liberar depoimentos
    NoListaDupla *atual = caderno->depoimentos.inicio;
    while (atual != NULL) {
        NoListaDupla *proximo = atual->prox;
        free(((Depoimento *)atual->dado)->local); // Liberar a string local
        free(((Depoimento *)atual->dado)->texto); // Liberar a string texto
        free(atual->dado); // Liberar o depoimento
        free(atual);       // Liberar o nó da lista
        atual = proximo;
    }

    // Liberar pistas
    atual = caderno->pistas.inicio;
    while (atual != NULL) {
        NoListaDupla *proximo = atual->prox;
        free(((Pista *)atual->dado)->local); // Liberar a string local
        free(((Pista *)atual->dado)->objeto); // Liberar a string objeto
        free(((Pista *)atual->dado)->descricao); // Liberar a string descricao
        Pista *pista = (Pista *)atual->dado;
        liberar_lista_dupla(pista->suspeitos_relacionados); // Liberar a lista de suspeitos relacionados
        free(pista);       // Liberar a pista
        free(atual);       // Liberar o nó da lista
        atual = proximo;
    }

    // Liberar reações
    atual = caderno->reacoes.inicio;
    while (atual != NULL) {
        NoListaDupla *proximo = atual->prox;
        free(((Reacao *)atual->dado)->quem_reagiu); // Liberar a string quem_reagiu
        free(atual->dado); // Liberar a reação
        free(atual);       // Liberar o nó da lista
        atual = proximo;
    }

    // Liberar contradições
    atual = caderno->contradicoes.inicio;
    while(atual != NULL){
        NoListaDupla * proximo = atual->prox;

        free(atual->dado);
        free(atual);

        atual = proximo;
    }

    free(caderno); // Liberar o caderno em si
}

CriterioFiltro* criar_criterio_vazio(void) {
    CriterioFiltro *criterio = malloc(sizeof *criterio);
    if (!criterio) return NULL;
    criterio->falante = criterio->assunto = criterio->dia = criterio->periodo = DESABILITADO;
    return criterio;
}

CriterioFiltro* criar_criterio(int falante, int assunto, int dia, int periodo) {
    CriterioFiltro *criterio = malloc(sizeof *criterio);
    if (!criterio) return NULL;
    criterio->falante = falante;
    criterio->assunto = assunto;
    criterio->dia = dia;
    criterio->periodo = periodo;
    return criterio;
}

int comparar_depoimentos_criterio(Depoimento *depoimento, CriterioFiltro *criterio) {
    if (!depoimento || !criterio) {
        return 0;
    }
    if (criterio->falante != DESABILITADO && depoimento->id_falante != criterio->falante) {
        return 0;
    }
    if (criterio->assunto != DESABILITADO && depoimento->id_assunto != criterio->assunto){
         return 0;
    }
    if (criterio->dia != DESABILITADO && depoimento->dia != criterio->dia){
        return 0;
    }
    if (criterio->periodo != DESABILITADO && (int)depoimento->periodo != criterio->periodo) {
        return 0;
    }
    return 1;
}

Lista *filtrar_depoimentos(Caderno *caderno, CriterioFiltro *criterio) {
    Lista *resultado = criar_lista_simples();
    if (!resultado){
        return NULL;
    }
    if (!caderno || !criterio){
        return resultado;   // caderno vazio/NULL não trava
    }

    int sequencia = 1;
    for (NoListaDupla *atual = caderno->depoimentos.inicio; atual != NULL; atual = atual->prox) {
        if (comparar_depoimentos_criterio((Depoimento *)atual->dado, criterio)) {
            lista_simples_inserir_no_fim(resultado, sequencia++, atual->dado);
        }
    }
    return resultado;
}
void liberar_resultado_filtro(Lista *resultado) {
    liberar_lista_simples(resultado);
}

Lista *depoimentos_do_suspeito(Caderno *caderno, int id_suspeito) {
    CriterioFiltro criterio = { id_suspeito, DESABILITADO, DESABILITADO, DESABILITADO };
    return filtrar_depoimentos(caderno, &criterio);
}
Lista *depoimentos_sobre_suspeito(Caderno *caderno, int id_suspeito) {
    CriterioFiltro criterio = { DESABILITADO, id_suspeito, DESABILITADO, DESABILITADO };
    return filtrar_depoimentos(caderno, &criterio);
}

int locais_conflitam(const Depoimento * A, const Depoimento * B){
    if(A == NULL || B == NULL){
        return 0;
    }
    if(A->local == NULL || B->local == NULL){
        return 0;
    }
    if(strcmp(A->local, "não informado") == 0 || strcmp(B->local, "não informado") == 0){
        return 0;
    }
    return strcmp(A->local, B->local) != 0;
}

int depoimentos_comparaveis(const Depoimento * A, const Depoimento * B){
    if(A == NULL || B == NULL){
        return 0;
    }
    if(A->id_assunto != B->id_assunto){
        return 0;
    }
    if(A->dia != B->dia){
        return 0;
    }
    if(A->periodo != B->periodo){
        return 0;
    }

    return 1;
}

int depoimentos_sao_contraditorios(const Depoimento * A, const Depoimento * B){
    if(!depoimentos_comparaveis(A, B)){
        return 0;
    }
    return locais_conflitam(A, B);
}

int registrar_contradicao(Caderno * caderno, Depoimento * A, Depoimento * B){
    if (caderno == NULL || A == NULL || B == NULL){
        return 0;
    }
    if (!depoimentos_sao_contraditorios(A, B)){
        return 0;
    }
    for(NoListaDupla * atual = caderno->contradicoes.inicio; atual != NULL; atual = atual->prox){
        Contradicao * c = atual->dado;

        int mesmo_par = (c->depoimento1->id_depoimento == A->id_depoimento &&
             c->depoimento2->id_depoimento == B->id_depoimento) ||
            (c->depoimento1->id_depoimento == B->id_depoimento &&
             c->depoimento2->id_depoimento == A->id_depoimento);
        if(mesmo_par == 1){
            return 0;
        }
    }
    Contradicao * nova = malloc(sizeof(Contradicao));
    if(nova == NULL){
        return 0;
    }
    nova->depoimento1 = A;
    nova->depoimento2 = B;

    int id = caderno->contradicoes.tamanho + 1;
    NoListaDupla * novo = malloc(sizeof(NoListaDupla));

    if(novo == NULL){
        free(nova);
        return 0;
    }
    novo->id = id;
    novo->dado = nova;
    novo->prox = NULL;
    novo->ant = caderno->contradicoes.fim;

    if(caderno->contradicoes.fim == NULL){
        caderno->contradicoes.inicio = novo;
    } else{
        caderno->contradicoes.fim->prox = novo;
    }

    caderno->contradicoes.fim = novo;
    caderno->contradicoes.tamanho ++;
    return 1;
}

int detectar_contradicoes_todas(Caderno *caderno){
    if(caderno == NULL){
        return 0;
    }

    int total_contradicoes = 0;

    for(NoListaDupla * atual = caderno->depoimentos.inicio; atual != NULL; atual = atual->prox){
        for(NoListaDupla * proximo = atual->prox; proximo != NULL; proximo = proximo->prox){
            Depoimento * A = atual->dado;
            Depoimento * B = proximo->dado;
            if(depoimentos_sao_contraditorios(A, B)){
                if(registrar_contradicao(caderno, A, B)){
                    total_contradicoes ++;
                }
            }
        }
    }

    return total_contradicoes;
}