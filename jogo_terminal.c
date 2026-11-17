#include "jogo_terminal.h"
#include "jogo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#endif

static int entrada_encerrada = 0;

// ---------------------------------------------------------------------------
// Entrada
// ---------------------------------------------------------------------------
static int ler_inteiro(void) {
    char linha[64];

    if (fgets(linha, sizeof linha, stdin) == NULL) {
        entrada_encerrada = 1;
        return -1;
    }

    char *fim = NULL;
    long valor = strtol(linha, &fim, 10);
    if (fim == linha) {
        return -1;
    }

    return (int)valor;
}

static void pausar(void) {
    printf("\n[Enter para continuar]");
    fflush(stdout);

    char linha[64];
    if (fgets(linha, sizeof linha, stdin) == NULL) {
        entrada_encerrada = 1;
    }
}

// ---------------------------------------------------------------------------
// Textos auxiliares
// ---------------------------------------------------------------------------
static const char *nome_suspeito(const Jogo *jogo, int id) {
    const Suspeito *suspeito = buscar_suspeito(jogo, id);
    return (suspeito != NULL) ? suspeito->nome : "Alguém";
}

static const char * nome_detetive(int id_detetive) {
    switch(id_detetive) {
        case ID_MISS_MARPLE:   
            return "Miss Marple";
        case ID_HERCULE_POIROT: 
            return "Hercule Poirot";
        default:               
            return "um detetive";
    }
}

static const char *nome_periodo(enum Periodo periodo) {
    switch (periodo) {
        case MANHA: return "manhã";
        case TARDE: return "tarde";
        case NOITE: return "noite";
        default:    return "?";
    }
}

static void mostrar_cabecalho(const Jogo *jogo) {
    printf("\n============================================================\n");
    printf(" Dia %d/%d  |  Fase: %-11s |  Credibilidade: %d\n",
           jogo->dia.dia_atual, TOTAL_DIAS, nome_fase(jogo->dia.fase_atual),
           jogo->credibilidade);
    printf(" Pontuação: %d\n", pontuacao_do_jogo(jogo));
    printf("============================================================\n");
}

static void listar_vivos(const Jogo *jogo) {
    const NoListaDuplaCircular *no = jogo->suspeitos->inicio;
    if (no == NULL) {
        return;
    }

    do {
        const Suspeito *suspeito = (const Suspeito *)no->dado;
        if (suspeito != NULL && suspeito->situacao == VIVO) {
            // A interface usa 1..6; o 0 fica reservado para cancelar/não prender.
            printf("  %d - %s\n", suspeito->id + 1, suspeito->nome);
        }
        no = no->prox;
    } while (no != jogo->suspeitos->inicio);
}

static void listar_todos_suspeitos(const Jogo *jogo) {
    const NoListaDuplaCircular *no = jogo->suspeitos->inicio;

    if (no == NULL) {
        return;
    }

    do {
        const Suspeito *suspeito = (const Suspeito *)no->dado;

        if (suspeito != NULL) {
            printf("  %d - %s\n", suspeito->id + 1, suspeito->nome);
        }

        no = no->prox;

    } while (no != jogo->suspeitos->inicio);
}

static void mostrar_reacoes(const Jogo *jogo, int id_depoimento) {
    const NoListaDupla *no = jogo->caderno->reacoes.inicio;
    if (no == NULL) {
        return;
    }

    while (no != NULL) {
        const Reacao *reacao = (const Reacao *)no->dado;
        if (reacao != NULL && reacao->id_depoimento == id_depoimento) {
            const char *tipo = (reacao->tipo_reacao == DISCORDA) ? "Discorda"
                             : (reacao->tipo_reacao == CONCORDA) ? "Concorda"
                             : "Neutro";
            printf("  - %s: %s\n", reacao->quem_reagiu, tipo);
        }
        no = no->prox;
    }
}

static void mostrar_depoimento(const Jogo *jogo, int id_depoimento) {
    const Depoimento *depoimento =
        buscar_depoimento_por_id(jogo->caderno, id_depoimento);
    if (depoimento == NULL) {
        return;
    }

    printf("\n  [Dia %d, %s] %s: \"%s\"\n",
           depoimento->dia, nome_periodo(depoimento->periodo),
           nome_suspeito(jogo, depoimento->id_falante), depoimento->texto);
    mostrar_reacoes(jogo, id_depoimento);
}

// ---------------------------------------------------------------------------
// Caderno
// ---------------------------------------------------------------------------
static void exibir_item_atual(const Jogo *jogo) {
    NoListaDupla *no = obter_item_atual(jogo->caderno);
    if (no == NULL) {
        printf("  (seção vazia)\n");
        return;
    }

    switch (jogo->caderno->secao_ativa) {
        case SECAO_DEPOIMENTOS: {
            const Depoimento *depoimento = (const Depoimento *)no->dado;
            printf("  Depoimento #%d — dia %d, %s\n",
                   depoimento->id_depoimento, depoimento->dia,
                   nome_periodo(depoimento->periodo));
            printf("  %s falou sobre %s, no %s:\n",
                   nome_suspeito(jogo, depoimento->id_falante),
                   nome_suspeito(jogo, depoimento->id_assunto),
                   depoimento->local);
            printf("  \"%s\"\n", depoimento->texto);
            break;
        }
        case SECAO_PISTAS: {
            const Pista *pista = (const Pista *)no->dado;
            printf("  Pista #%d — dia %d, %s\n", no->id, pista->dia, pista->local);
            printf("  Objeto: %s\n", pista->objeto);
            printf("  %s\n", pista->descricao);
            break;
        }
        case SECAO_REACOES: {
            const Reacao *reacao = (const Reacao *)no->dado;
            const char *tipo = (reacao->tipo_reacao == DISCORDA) ? "discordou"
                             : (reacao->tipo_reacao == CONCORDA) ? "concordou"
                             : "ficou neutro";
            printf("  Reação #%d — dia %d: %s %s do depoimento #%d\n",
                   no->id, reacao->dia, reacao->quem_reagiu, tipo,
                   reacao->id_depoimento);
            break;
        }
        case SECAO_CONTRADICOES: {
            const Contradicao *contradicao = (const Contradicao *)no->dado;
            printf("  Contradição #%d\n", no->id);
            printf("  Depoimento #%d: \"%s\"\n",
                   contradicao->depoimento1->id_depoimento,
                   contradicao->depoimento1->texto);
            printf("  Depoimento #%d: \"%s\"\n",
                   contradicao->depoimento2->id_depoimento,
                   contradicao->depoimento2->texto);
            break;
        }
        default:
            break;
    }
}

static void navegar_secao(Jogo *jogo, SecaoCaderno secao) {
    selecionar_secao(jogo->caderno, secao);

    while (!entrada_encerrada) {
        printf("\n");
        exibir_item_atual(jogo);
        printf("\n  [1] próximo  [2] anterior  [0] voltar\n> ");

        int opcao = ler_inteiro();
        if (opcao == 1) {
            avancar_secao(jogo->caderno);
        } else if (opcao == 2) {
            voltar_secao(jogo->caderno);
        } else if (opcao == 0) {
            return;
        }
    }
}

static void menu_ordenar_pistas(Jogo *jogo) {
    Caderno *caderno = jogo->caderno;

    if (caderno->pistas.tamanho == 0) {
        printf("\nNenhuma pista registrada para ordenar.\n");
        pausar();
        return;
    }

    printf("\n--- ORDENAR PISTAS ---\n");
    printf("  [1] Local (A-Z)\n");
    printf("  [2] Objeto (A-Z)\n");
    printf("  [3] Dia (mais recente primeiro)\n");
    printf("  [0] Cancelar\n> ");

    int opcao = ler_inteiro();
    CriterioOrdenacao criterio;

    switch (opcao) {
        case 1:
            criterio = ORDENAR_POR_LOCAL;
            break;

        case 2:
            criterio = ORDENAR_POR_OBJETO;
            break;

        case 3:
            criterio = ORDENAR_POR_DIA_RECENTE;
            break;

        case 0:
            return;

        default:
            printf("\nOpcao invalida.\n");
            pausar();
            return;
    }

    printf("\n--- ANTES DA ORDENACAO ---\n");

    for (NoListaDupla *atual = caderno->pistas.inicio;
         atual != NULL;
         atual = atual->prox) {

        Pista *pista = (Pista *)atual->dado;

        printf("Pista %d | Dia %d | Local: %s | Objeto: %s\n",
               atual->id,
               pista->dia,
               pista->local,
               pista->objeto);
    }

    int trocas = ordenar_pistas(caderno, criterio);

    printf("\n--- DEPOIS DA ORDENACAO ---\n");

    for (NoListaDupla *atual = caderno->pistas.inicio;
         atual != NULL;
         atual = atual->prox) {

        Pista *pista = (Pista *)atual->dado;

        printf("Pista %d | Dia %d | Local: %s | Objeto: %s\n",
               atual->id,
               pista->dia,
               pista->local,
               pista->objeto);
    }

    printf("\nTotal de trocas realizadas: %d\n", trocas);

    pausar();
}

static void menu_depoimentos_suspeito(Jogo *jogo) {
    printf("\n--- DEPOIMENTOS SOBRE UM SUSPEITO ---\n");

    listar_todos_suspeitos(jogo);

    printf("  0 - Voltar\n");
    printf("\nEscolha um suspeito: ");

    int opcao = ler_inteiro();

    if (opcao == 0) {
        return;
    }

    int id_suspeito = opcao - 1;
    int suspeito_existe = 0;

    const NoListaDuplaCircular *no = jogo->suspeitos->inicio;

    if (no != NULL) {
        do {
            const Suspeito *suspeito = (const Suspeito *)no->dado;

            if (suspeito != NULL && suspeito->id == id_suspeito) {
                suspeito_existe = 1;
                break;
            }

            no = no->prox;
        } while (no != jogo->suspeitos->inicio);
    }

    if (!suspeito_existe) {
        printf("\nSuspeito invalido.\n");
        pausar();
        return;
    }

    Lista *resultado = depoimentos_sobre_suspeito(
        jogo->caderno, id_suspeito
    );

    if (resultado == NULL) {
        printf("\nErro ao consultar depoimentos.\n");
        pausar();
        return;
    }

    if (resultado->inicio == NULL) {
        printf("\nNenhum depoimento encontrado sobre esse suspeito.\n");
    } else {
        int quantidade = 0;

        for (NoLista *atual = resultado->inicio;
             atual != NULL;
             atual = atual->prox) {

            Depoimento *depoimento = (Depoimento *)atual->dado;

            printf("\nDepoimento %d\n", depoimento->id_depoimento);
            printf("Dia: %d\n", depoimento->dia);
            printf("Periodo: %s\n", nome_periodo(depoimento->periodo));
            printf("Falante: %s\n", nome_suspeito(jogo, depoimento->id_falante));
            printf("Local: %s\n",
                   depoimento->local != NULL ? depoimento->local : "Nao informado");
            printf("Texto: %s\n",
                   depoimento->texto != NULL ? depoimento->texto : "Sem texto");

            quantidade++;
        }

        printf("\nTotal de depoimentos encontrados: %d\n", quantidade);
    }

    liberar_resultado_filtro(resultado);
    pausar();
}

static void ver_caderno(Jogo *jogo) {
    while (!entrada_encerrada) {
        printf("\n--- CADERNO ---\n");
        printf("  [1] Depoimentos (%d)\n", jogo->caderno->depoimentos.tamanho);
        printf("  [2] Pistas (%d)\n", jogo->caderno->pistas.tamanho);
        printf("  [3] Reações (%d)\n", jogo->caderno->reacoes.tamanho);
        printf("  [4] Contradições (%d)\n", jogo->caderno->contradicoes.tamanho);
        printf("  [5] Ordenar pistas\n");
        printf("  [6] Depoimentos sobre um suspeito\n");
        printf("  [0] Voltar\n> ");

        switch (ler_inteiro()) {
            case 1: navegar_secao(jogo, SECAO_DEPOIMENTOS); break;
            case 2: navegar_secao(jogo, SECAO_PISTAS); break;
            case 3: navegar_secao(jogo, SECAO_REACOES); break;
            case 4: navegar_secao(jogo, SECAO_CONTRADICOES); break;
            case 5: menu_ordenar_pistas(jogo);break;
            case 6: menu_depoimentos_suspeito(jogo);break;
            case 0: return;
            default: break;
        }
    }
}

// ---------------------------------------------------------------------------
// Fases
// ---------------------------------------------------------------------------
static int escolher_alvo(Jogo *jogo) {
    while (!entrada_encerrada) {
        printf("\nSobre quem interrogar?\n");
        listar_vivos(jogo);
        printf("  (escolha o próprio suspeito para ouvir o álibi dele)\n");
        printf("  0 - Cancelar\n> ");

        int escolha = ler_inteiro();
        if (escolha == 0) {
            return -1;
        }

        int id = escolha - 1;
        const Suspeito *alvo = buscar_suspeito(jogo, id);
        if (alvo != NULL && alvo->situacao == VIVO) {
            return id;
        }

        printf("Opção inválida.\n");
    }
    return -1;
}

static int escolher_periodo(enum Periodo *periodo) {
    if (periodo == NULL) {
        return 0;
    }

    while (!entrada_encerrada) {
        printf("\nEm qual período interrogar?\n");
        printf("  [1] Manhã\n");
        printf("  [2] Tarde\n");
        printf("  [3] Noite\n");
        printf("  [0] Cancelar\n> ");

        switch (ler_inteiro()) {
            case 0: return 0;
            case 1: *periodo = MANHA; return 1;
            case 2: *periodo = TARDE; return 1;
            case 3: *periodo = NOITE; return 1;
            default: printf("Opção inválida.\n"); break;
        }
    }

    return 0;
}

static void jogar_manha(Jogo *jogo) {
    printf("\nMANHÃ — O turno começa e a fábrica comenta o caso.\n");
    printf("  Pista do dia: %s\n", nome_ultima_pista(jogo));
    pausar();

    if (!entrada_encerrada) {
        avancar_fase_jogo(jogo);
    }
}

static void jogar_debate(Jogo *jogo) {
    Suspeito *falante = suspeito_do_turno(jogo);

    if (falante == NULL) {
        printf("\nTodos os suspeitos vivos já falaram hoje.\n");
        pausar();
        if (!entrada_encerrada) {
            avancar_fase_jogo(jogo);
        }
        return;
    }

    printf("\nDEBATE — vez de %s.\n", falante->nome);
    printf("  [1] Interrogar\n");
    printf("  [2] Abrir caderno\n");
    printf("  [3] Encerrar debate\n> ");

    switch (ler_inteiro()) {
        case 1: {
            enum Periodo periodo;
            if (!escolher_periodo(&periodo)) {
                break;
            }

            printf("\nPeríodo escolhido: %s.\n", nome_periodo(periodo));

            int alvo = escolher_alvo(jogo);
            if (alvo >= 0) {
                int id_depoimento = perguntar_sobre(jogo, alvo, periodo);
                if (id_depoimento > 0) {
                    mostrar_depoimento(jogo, id_depoimento);
                    pausar();
                } else {
                    printf("Não foi possível registrar o depoimento.\n");
                }
            }
            break;
        }
        case 2:
            ver_caderno(jogo);
            break;
        case 3:
            printf("\nVocê encerra o debate.\n");
            pausar();
            if (!entrada_encerrada) {
                avancar_fase_jogo(jogo);
            }
            break;
        default:
            printf("Opção inválida.\n");
            break;
    }
}

static void jogar_julgamento(Jogo *jogo) {
    while (!entrada_encerrada) {
        printf("\nJULGAMENTO — quem você vai prender?\n");
        listar_vivos(jogo);
        printf("  0 - Não prender ninguém\n> ");

        int escolha = ler_inteiro();
        if (escolha == 0) {
            printf("\nVocê decide não prender ninguém hoje.\n");
            pausar();
            if (!entrada_encerrada) {
                avancar_fase_jogo(jogo);
            }
            return;
        }

        int id = escolha - 1;
        Suspeito *suspeito = buscar_suspeito(jogo, id);
        if (suspeito == NULL || suspeito->situacao != VIVO) {
            printf("Opção inválida.\n");
            continue;
        }

        if (!prender_suspeito(jogo, id)) {
            printf("Não foi possível prender esse suspeito.\n");
            continue;
        }

        if (jogo->resultado == JOGO_VITORIA) {
            printf("\nVocê prendeu %s. É o assassino!\n", suspeito->nome);
        } else if (suspeito->papel == CUMPLICE) {
            printf("\nVocê prendeu %s. Ele não é o assassino, mas decide falar.\n",
                   suspeito->nome);
            printf("%s\n", revelacao_do_cumplice(jogo));
            printf("Credibilidade agora: %d\n", jogo->credibilidade);
        } else {
            printf("\nVocê prendeu %s, mas as provas não se sustentam.\n",
                   suspeito->nome);
            printf("Credibilidade agora: %d\n", jogo->credibilidade);
        }
        pausar();

        if (!entrada_encerrada && !jogo_terminou(jogo)) {
            avancar_fase_jogo(jogo);
        }
        return;
    }
}

static void jogar_noite(Jogo *jogo) {
    printf("\nNOITE — a fábrica fica em silêncio.\n");

    if (jogo->ultima_vitima != NENHUM) {
        printf("  %s foi encontrado morto na madrugada.\n",
               nome_suspeito(jogo, jogo->ultima_vitima));
    } else {
        printf("  Ninguém morreu esta noite.\n");
    }

    pausar();
    if (!entrada_encerrada) {
        avancar_fase_jogo(jogo);
    }
}

// ---------------------------------------------------------------------------
// Partida
// ---------------------------------------------------------------------------
static void mostrar_intro(void) {
    printf("============================================================\n");
    printf(" ÚLTIMO TURNO\n");
    printf("============================================================\n");

    printf("\n Na década de 1930, em uma fábrica industrial marcada por uma\n");
    printf(" rotina rígida de trabalho, conflitos internos e relações de\n");
    printf(" hierarquia, um acontecimento inesperado interrompe o\n");
    printf(" funcionamento normal da produção.\n");

    printf("\n Ao final de mais um expediente, um operário da fábrica é\n");
    printf(" encontrado morto na sala de máquinas. As circunstâncias de\n");
    printf(" sua morte são misteriosas, e ninguém parece saber exatamente\n");
    printf(" o que aconteceu.\n");
}

static void mostrar_intro_restante(int detetive) {
    printf("\n A investigação fica sob responsabilidade de %s, que precisa\n",
           nome_detetive(detetive));
    printf(" descobrir quem está por trás do assassinato antes que seja\n");
    printf(" tarde demais.\n");

    printf("\n O assassino continua agindo durante as noites, e novas mortes\n");
    printf(" tornam a investigação cada vez mais urgente. A cada dia, os\n");
    printf(" sobreviventes são interrogados, apresentam suas versões dos\n");
    printf(" acontecimentos e reagem aos depoimentos uns dos outros.\n");

    printf("\n Você tem %d dias para descobrir o culpado.\n", TOTAL_DIAS);
    printf("============================================================\n");
}

static int escolher_detetive(void) {
    while (!entrada_encerrada) {
        printf("\nQuem conduz a investigação?\n");
        printf("  1 - Miss Marple\n");
        printf("  2 - Hercule Poirot\n> ");

        switch (ler_inteiro()) {
            case 1: return ID_MISS_MARPLE;
            case 2: return ID_HERCULE_POIROT;
            default: printf("Opção inválida.\n"); break;
        }
    }
    return -1;
}

static void mostrar_resultado(const Jogo *jogo) {
    printf("\n============================================================\n");
    switch (jogo->resultado) {
        case JOGO_VITORIA:
            printf(" VITÓRIA! Você prendeu o assassino.\n");
            break;
        case JOGO_DERROTA_CREDIBILIDADE:
            printf(" DERROTA: sua credibilidade chegou a zero.\n");
            break;
        case JOGO_DERROTA_TEMPO:
            printf(" DERROTA: os %d dias acabaram.\n", TOTAL_DIAS);
            break;
        default:
            printf(" A investigação terminou.\n");
            break;
    }
    printf("============================================================\n");

    // Texto narrativo do final (vitória, derrota por credibilidade ou por tempo).
    const char *final = texto_final(jogo);
    if (final != NULL && final[0] != '\0') {
        printf("\n%s\n", final);
    }

    const Suspeito *assassino = buscar_suspeito(jogo, jogo->papeis.id_assassino);
    const Suspeito *cumplice = buscar_suspeito(jogo, jogo->papeis.id_cumplice);
    int contradicoes = (jogo->caderno != NULL) ? jogo->caderno->contradicoes.tamanho : 0;

    printf("\n------------------------------------------------------------\n");
    printf(" Assassino: %s\n", (assassino != NULL) ? assassino->nome : "?");
    if (cumplice != NULL) {
        printf(" Cúmplice: %s\n", cumplice->nome);
    }
    printf(" Dias usados: %d/%d\n", jogo->dia.dia_atual, TOTAL_DIAS);
    printf(" Credibilidade restante: %d\n", jogo->credibilidade);
    printf(" Contradições encontradas: %d\n", contradicoes);
    printf(" PONTUAÇÃO FINAL: %d\n", pontuacao_do_jogo(jogo));
    printf("============================================================\n");
}

int executar_jogo_terminal(void) {
#if defined(_WIN32)
    // O jogo imprime textos em UTF-8. Sem isto, os acentos aparecem quebrados no console do Windows (a página de código padrão não é UTF-8).
    SetConsoleOutputCP(CP_UTF8);
#endif

    entrada_encerrada = 0;

    mostrar_intro();

    int detetive = escolher_detetive();
    if (entrada_encerrada || detetive < 0) {
        return 0;
    }

    mostrar_intro_restante(detetive);
    pausar();
    if (entrada_encerrada) {
        return 0;
    }

    Jogo jogo;
    if (!iniciar_jogo(&jogo, detetive)) {
        fprintf(stderr, "Erro: não foi possível iniciar a partida.\n");
        return 1;
    }

    while (!jogo_terminou(&jogo) && !entrada_encerrada) {
        mostrar_cabecalho(&jogo);

        switch (jogo.dia.fase_atual) {
            case FASE_MANHA:      jogar_manha(&jogo); break;
            case FASE_DEBATE:     jogar_debate(&jogo); break;
            case FASE_JULGAMENTO: jogar_julgamento(&jogo); break;
            case FASE_NOITE:      jogar_noite(&jogo); break;
            case FASE_FINALIZADO: break;
            default:              break;
        }
    }

    if (!entrada_encerrada) {
        mostrar_resultado(&jogo);
    }

    liberar_jogo(&jogo);
    return 0;
}
