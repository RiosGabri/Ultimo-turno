# Planejamento — Tela do debate (raylib) · Issue #21

> **Issues relacionadas:** #21 (esta), #13 (agenda/períodos), #17 (roda do debate), #19 (helpers de UI), #20 (fluxo de telas), #22 (caderno), #23 (julgamento/noite/fim).
>
> **Última atualização:** 17/11/2026.
>
> Este documento detalha apenas a parte de **interface gráfica do debate**. O planejamento geral de #17 + #21 continua em `PLANEJAMENTO.md`.

---

## 1. Situação atual

Branch de trabalho: `feat/debate`, já com a `main` e a `feat/telas` mergeadas.

| Item | Situação |
|---|---|
| #13 — agenda, períodos e mentira só à noite | ✅ Na branch |
| #17 — roda do debate (início avança por dia) | ✅ Na branch |
| #19 — helpers de UI (`ui.c`/`ui.h`) | ✅ Na branch |
| #20 — fluxo de telas | ❌ Aberta; `main.c` só tem o menu |
| #22 — tela do caderno | ❌ Aberta; usar placeholder |
| #23 — julgamento, noite e fim | ❌ Aberta; usar placeholder |
| Sprites e textos finais | ⏳ Em produção; usar placeholders com fallback |

O que já existe:

- `interface/ui.c` / `ui.h`: fonte com acentos, medição, quebra de linha, painel de texto, botões com mouse/teclado e ícones desenhados com formas.
- `assets/fontes/DejaVuSans.ttf`.
- `assets/image/fabrica.png` e `assets/logo.png`.
- Motor do debate exposto por `jogo.h`.
- Terminal (`jogo_terminal.c`) com a lógica que a tela deve espelhar.

O que **não** existe:

- `interface/tela_debate.c` / `.h`.
- Qualquer fluxo de telas além do menu.
- Tela de caderno e telas de julgamento/noite/fim.

---

## 2. Escopo da #21

Implementar uma tela raylib que permita:

1. Mostrar o suspeito da vez.
2. Escolher um alvo vivo e um período (manhã, tarde ou noite).
3. Chamar `perguntar_sobre` e exibir o depoimento.
4. Exibir as reações dos demais suspeitos com ícones.
5. Avisar quando uma nova contradição for registrada.
6. Abrir o caderno (placeholder até a #22).
7. Encerrar o debate e avançar a fase.

Fora do escopo desta issue:

- Tela real do caderno (#22).
- Julgamento, noite e fim (#23).
- Fluxo completo de telas e transições (#20).
- Arte final dos personagens e cenários.

---

## 3. Arquivos a criar/alterar

### Criar

| Arquivo | Conteúdo |
|---|---|
| `interface/tela_debate.h` | Tipos `EtapaDebate`, `AcaoDebate`, `TelaDebate` e protótipos |
| `interface/tela_debate.c` | Estado, atualização e desenho da tela |
| `test/teste_tela_debate.c` (opcional) | Testes da lógica de seleção/ação sem abrir janela |

### Alterar

| Arquivo | Alteração |
|---|---|
| `main.c` | Entrada temporária: menu → partida → tela de debate |
| `CMakeLists.txt` | Adicionar `interface/tela_debate.c` ao `add_executable(jogo ...)` |
| `Makefile` | Adicionar `interface/tela_debate.c` a `JOGO_SRCS` (ou usar `$(wildcard interface/*.c)`) |
| `CMakeLists.txt` / `Makefile` | Registrar o teste, se ele existir |

---

## 4. Modelo de estado

Separar a lógica da tela do estado do jogo. A tela guarda apenas o que é transitório de interface.

```c
typedef enum {
    DEBATE_ESCOLHENDO_ALVO,
    DEBATE_ESCOLHENDO_PERIODO,
    DEBATE_MOSTRANDO_DEPOIMENTO
} EtapaDebate;

typedef enum {
    TELA_DEBATE_CONTINUAR,
    TELA_DEBATE_ABRIR_CADERNO,
    TELA_DEBATE_ENCERRAR
} AcaoDebate;

typedef struct {
    Jogo *jogo;

    EtapaDebate etapa;
    int id_alvo;
    int id_depoimento;

    // Controle do aviso de nova contradição.
    int contradicoes_antes;
    float tempo_alerta;

    // Navegação por teclado/mouse.
    int botao_focado;
    bool interrogar_pressionado;

    // Estado visual dos botões (BotaoUI).
    BotaoUI botao_interrogar;
    BotaoUI botao_caderno;
    BotaoUI botao_encerrar;
    BotaoUI botoes_alvo[TOTAL_SUSPEITOS];
    BotaoUI botoes_periodo[TOTAL_PERIODOS];
} TelaDebate;
```

### API proposta

```c
void tela_debate_iniciar(TelaDebate *tela, Jogo *jogo);
AcaoDebate tela_debate_atualizar(TelaDebate *tela);
void tela_debate_desenhar(const TelaDebate *tela, const UIFonte *fonte);
```

`tela_debate_atualizar` lê mouse/teclado e devolve uma `AcaoDebate`:

- `TELA_DEBATE_CONTINUAR`: permanece na tela.
- `TELA_DEBATE_ABRIR_CADERNO`: o `main.c` mostra o placeholder do caderno.
- `TELA_DEBATE_ENCERRAR`: o `main.c` chama `avancar_fase_jogo`.

---

## 5. Comportamento da tela

### 5.1 Cabeçalho

- Título “Debate”.
- Dia atual e fase (`jogo.dia.dia_atual`, `nome_fase`).
- Suspeito da vez: `suspeito_do_turno(&jogo)`.
- Se `suspeito_do_turno` for `NULL`: mostrar “Todos os suspeitos vivos já falaram hoje” e deixar apenas **Encerrar debate** habilitado.

### 5.2 Escolha do alvo

- Listar somente suspeitos com `situacao == VIVO`.
- Mortos e presos não aparecem como opção.
- Cada item mostra nome e cargo.
- O jogador pode escolher o próprio suspeito da vez para ouvir o álibi dele.
- Um botão/atalho de **Cancelar** volta para o estado anterior.

### 5.3 Escolha do período

Três botões: **Manhã**, **Tarde**, **Noite**.

- O período é obrigatório antes de perguntar, como no terminal.
- A escolha não é persistida; vale só para a pergunta atual.

### 5.4 Depoimento

Depois de `perguntar_sobre(tela->jogo, alvo, periodo)`:

- Buscar o depoimento com `buscar_depoimento_por_id`.
- Exibir:
  - quem falou (`Depoimento->id_falante` → nome);
  - período (`nome_periodo`);
  - local afirmado (`Depoimento->local`);
  - texto completo (`Depoimento->texto`), com quebra de linha e painel.
- Exibir as reações percorrendo `jogo->caderno->reacoes`:
  - filtrar por `reacao->id_depoimento == id_depoimento`;
  - `CONCORDA` → `UI_ICONE_CONCORDA`;
  - `DISCORDA` → `UI_ICONE_DISCORDA`;
  - `NEUTRO` → `UI_ICONE_NEUTRO`;
  - mostrar também `quem_reagiu`.

### 5.5 Aviso de contradição

Antes de chamar `perguntar_sobre`, guardar:

```c
int antes = tela->jogo->caderno->contradicoes.tamanho;
```

Depois da chamada, comparar com o novo tamanho. Se aumentou:

- exibir ícone `UI_ICONE_ALERTA`;
- mostrar “Nova contradição registrada no caderno!”;
- manter o aviso por alguns segundos (`tempo_alerta`).

### 5.6 Botões

| Botão | Quando habilita | Ação |
|---|---|---|
| **Interrogar** | Há alvo e período escolhidos e `suspeito_do_turno != NULL` | `perguntar_sobre` e vai para o depoimento |
| **Abrir caderno** | Sempre | Devolve `TELA_DEBATE_ABRIR_CADERNO` |
| **Encerrar debate** | Sempre | Devolve `TELA_DEBATE_ENCERRAR` |

Regra de decisão: **Encerrar debate é sempre permitido**, como no terminal. `debate_encerrado` só indica o fim natural da volta; não bloqueia o botão.

---

## 6. Placeholders

Enquanto os assets e as telas #22/#23 não existem:

- **Fundo:** `assets/image/fabrica.png`.
- **Retrato do suspeito:** tentar `LoadTexture` de um caminho previsto; se falhar, desenhar retângulo com a inicial do nome e o cargo.
- **Caderno:** painel “Caderno em construção” com botão **Voltar** para o debate.
- **Julgamento/noite/fim:** após **Encerrar debate**, mostrar painel “Tela em construção” e oferecer **Avançar** (chama `avancar_fase_jogo`).
- **Textos finais:** centralizar em constantes no topo de `tela_debate.c` para facilitar a troca posterior.

Sugestão de organização:

```c
// tela_debate.c — pontos de troca de assets/textos
#define DEBATE_FUNDO_PADRAO "assets/image/fabrica.png"
#define DEBATE_TEXTO_VAZIO  "Nenhum depoimento ainda."
```

---

## 7. Integração temporária no `main.c` (até a #20)

Como a #20 não existe, criar um caminho mínimo e explicitamente temporário:

1. **Iniciar** → `iniciar_jogo(&jogo, ID_MISS_MARPLE)`.
2. Loop principal lê `jogo.dia.fase_atual`:
   - `FASE_DEBATE` → `tela_debate_*`;
   - `FASE_MANHA`, `FASE_JULGAMENTO`, `FASE_NOITE`, `FASE_FINALIZADO` → painel provisório com botão **Avançar**;
   - ao sair do loop, `liberar_jogo(&jogo)`.
3. **Abrir caderno** → placeholder com **Voltar**.
4. **Encerrar debate** → `avancar_fase_jogo(&jogo)`.

Quando a #20 entrar, substitui-se apenas essa entrada/fluxo; a `tela_debate` permanece.

---

## 8. Build

### CMake

Adicionar `interface/tela_debate.c` ao alvo gráfico:

```cmake
add_executable(jogo
    main.c
    interface/ui.c
    interface/tela_debate.c
)
```

### Makefile

Hoje:

```make
JOGO_SRCS := $(wildcard *.c) interface/ui.c $(JOGO_ESTRUTURAS)
```

Adicionar explicitamente:

```make
JOGO_SRCS := $(wildcard *.c) interface/ui.c interface/tela_debate.c $(JOGO_ESTRUTURAS)
```

Alternativa: trocar por `$(wildcard interface/*.c)` para não precisar editar a cada tela nova.

---

## 9. Testes

Seguir o padrão da #19: separar a lógica de entrada (`EntradaBotao`) do desenho, para poder testar sem abrir janela.

Casos mínimos:

- [ ] Não permitir interrogar alvo morto.
- [ ] Não permitir interrogar alvo preso.
- [ ] Exigir período válido antes de interrogar.
- [ ] Com `suspeito_do_turno == NULL`, **Interrogar** fica desabilitado.
- [ ] **Encerrar debate** continua habilitado com a roda encerrada.
- [ ] `perguntar_sobre` avança o turno e atualiza o falante.
- [ ] Contradição nova é detectada comparando `contradicoes.tamanho`.
- [ ] Depoimento retornado é o mesmo que aparece na tela.

Se o teste exigir raylib apenas para tipos (`Rectangle`), compilar como `teste_ui`: sem abrir janela.

---

## 10. Passo a passo de implementação

1. Criar `interface/tela_debate.h` com os tipos e a API.
2. Criar `interface/tela_debate.c` com:
   - inicialização dos botões;
   - máquina de estados (alvo → período → depoimento);
   - leitura de entrada e devolução de `AcaoDebate`;
   - desenho com `ui.h`.
3. Implementar o aviso de contradição.
4. Implementar os placeholders de caderno e fases seguintes.
5. Ligar no `main.c` com a entrada temporária.
6. Adicionar ao `CMakeLists.txt` e ao `Makefile`.
7. Escrever os testes de lógica.
8. Rodar `make test`, `ctest`, `make jogo` e `make terminal`.
9. Validar manualmente: interrogar em cada período, ver reações, gerar contradição, encerrar debate.

---

## 11. Critérios de aceite da #21

- [ ] Mostra o suspeito da vez.
- [ ] Permite escolher alvo vivo e período.
- [ ] Chama `perguntar_sobre` e mostra o depoimento.
- [ ] Mostra reações com os ícones de concorda/discorda/neutro.
- [ ] Avisa quando uma nova contradição é registrada.
- [ ] Botões **Abrir caderno** e **Encerrar debate** funcionam; **Encerrar debate** está sempre disponível.
- [ ] Tela funciona com placeholders, sem depender de assets finais.
- [ ] `make test` e `ctest` passam.
- [ ] Build gráfico (`make jogo`) compila sem warnings novos.

---

## 12. Riscos e observações

| Risco | Mitigação |
|---|---|
| #20 atrasar | Entrada temporária no menu, isolada para ser removida depois |
| #22/#23 atrasarem | Placeholders com botão de voltar/avançar |
| Assets finais atrasarem | Fallback desenhado com formas e iniciais |
| Layout estourar em textos longos | Usar `ui_desenhar_painel_texto` e `ui_quebrar_texto` |
| Duplicar lógica do terminal | Reutilizar apenas as funções do motor; a tela não deve recalcular regras |
| Teste depender de raylib/janela | Separar entrada da lógica, como na #19 |

---

## 13. Referências rápidas

### Motor

- `jogo.h` / `jogo.c`
  - `suspeito_do_turno`
  - `perguntar_sobre`
  - `debate_encerrado`
  - `buscar_suspeito`
  - `local_do_suspeito` / `local_do_crime`
  - `avancar_fase_jogo`
  - `nome_fase`
- `estruturas/entidades/caderno.h`
  - `buscar_depoimento_por_id`
  - `Secao reacoes` e `Secao contradicoes`
  - `Depoimento`, `Reacao`, `TipoReacao`
- `dia.h`
  - `FaseDia`, `TOTAL_DIAS`, `FASE_DEBATE`

### Interface

- `interface/ui.h`
  - `UIFonte`, `ui_carregar_fonte`, `ui_descarregar_fonte`
  - `ui_desenhar_painel_texto`
  - `ui_desenhar_texto_quebrado`
  - `BotaoUI`, `ui_desenhar_botao`, `ui_navegar_botoes`
  - `UIIcone`, `ui_desenhar_icone`
- `jogo_terminal.c`
  - `jogar_debate` (fluxo de referência)
  - `mostrar_depoimento`
  - `mostrar_reacoes`
  - `escolher_alvo`
