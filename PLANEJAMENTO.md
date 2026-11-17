# Planejamento — Issues #17 e #21

> **Status:** planejado, ainda não implementado.
>
> **Última atualização:** 17/11/2026.
>
> Este documento cobre as duas issues anexadas ao repositório:
>
> - `issue15.md` → issue **#17** do GitHub — *Roda do debate: ajustes*;
> - `issue16.md` → issue **#21** do GitHub — *Tela do debate (raylib)*.

---

## 1. Mapeamento das issues

| Arquivo local | Issue real | Título | Dependências |
|---|---|---|---|
| `issue15.md` | **#17** | Roda do debate: ajustes | Nenhuma; pode ser feita direto na `main` |
| `issue16.md` | **#21** | Tela do debate (raylib) | **#13, #19 e #20** |

### Situação atual do repositório

- A branch local `feat/debate` está no mesmo commit da `main`.
- A `main` **não tem** a agenda/períodos da #13.
- A #13 está no PR draft #39, na branch `origin/feat/jogo-dedutivel`.
- As issues #19 (helpers de UI) e #20 (fluxo de telas) ainda não têm branch/PR.
- `main.c` contém apenas o menu inicial em raylib.
- `assets/` contém apenas `logo.png`; não há fonte `.ttf`.
- O terminal já tem a lógica que a tela gráfica deve espelhar.

---

## 2. Decisões tomadas

| Tema | Decisão |
|---|---|
| Ordem de execução | Fazer a **#17 agora**; a **#21 somente depois** que #13, #19 e #20 entrarem na `main` |
| Mortos e presos na roda | **Manter na lista e pular** pela `situacao`; não remover da lista |
| Caderno na tela de debate | **Placeholder com botão de voltar** até a #22 existir |
| Encerrar debate | **Sempre permitido**, como no terminal; `debate_encerrado` indica o fim natural da volta |

### Justificativa para manter mortos e presos na lista

A lista circular de suspeitos é usada como o **elenco completo** do jogo, não apenas como a roda do debate. Ela alimenta:

- `buscar_suspeito`;
- a tela de resultado;
- a revelação do assassino e do cúmplice;
- o crime noturno;
- o caderno e consultas por suspeito.

Remover nós da lista quebraria essas consultas ou exigiria manter uma segunda lista apenas para a roda. Como a issue #17 permite explicitamente continuar pulando pela situação, essa é a opção de menor risco.

A função `lista_dupla_circular_remover` continua coberta pelos testes unitários em `test/teste_lista_circular.c` (remoção do meio, do início e até sobrar 1 nó).

---

## 3. Diagnóstico técnico

### 3.1 Motor do debate (#17)

- `jogo.c:159` — `avancar_turno` começa em `jogo->turno_atual->prox` ou, quando `turno_atual == NULL`, em `jogo->suspeitos->inicio`.
- `jogo.c:359` — `iniciar_debate` zera `interrogados`, zera `turno_atual` e chama `avancar_turno`. Resultado: todo dia começa no primeiro vivo da lista, normalmente o Gerente.
- `jogo.c:369` — `suspeito_do_turno` devolve o suspeito atual.
- `jogo.c:405` — `perguntar_sobre` marca o falante em `jogo->interrogados` e avança o turno.
- `jogo.h:35` — o estado do debate é apenas `turno_atual` + bitmap `interrogados`.
- `jogo.c:195` — `realizar_crime_noturno` mata inocentes durante a noite; mortos e presos continuam na lista.
- `test/teste_lista_circular.c:122-179` — já cobre remoção do meio, do início e até sobrar 1 nó.

### 3.2 UI (#21)

- `main.c` só tem o menu inicial em raylib. Não existe tela de jogo, estado de tela, fonte customizada nem pasta `interface/`.
- `assets/` só tem `logo.png`; não há `.ttf`.
- O terminal já tem a lógica que a tela deve espelhar:
  - `jogo_terminal.c:118` — `mostrar_reacoes`;
  - `jogo_terminal.c:136` — `mostrar_depoimento`;
  - `jogo_terminal.c:437` — `jogar_debate`.
- O caderno já registra contradições dentro de `registrar_depoimento` (`caderno.c:45-94` e `caderno.c:569-615`), então a UI só precisa comparar `jogo->caderno->contradicoes.tamanho` antes/depois.
- `CMakeLists.txt:83` e `Makefile:34` precisam incluir os novos arquivos da pasta `interface/` no alvo gráfico.

### 3.3 Dependências da #21

- **#13:** muda `perguntar_sobre` para receber `enum Periodo`, adiciona agenda e período no depoimento.
- **#19:** cria `interface/ui.c` e `interface/ui.h` com fonte, quebra de linha, botão, painel e ícones.
- **#20:** cria o fluxo de telas (menu → detetive → introdução → manhã → debate etc.).
- **#22:** tela do caderno; a #21 só precisa do botão “Abrir caderno”.

---

## 4. Plano em etapas

### Etapa 0 — Alinhar a base e o escopo

1. Confirmar que `issue15.md` = #17 e `issue16.md` = #21.
2. Para a #17: criar uma branch nova a partir da `main`, por exemplo:
   ```sh
   git switch main
   git pull
   git switch -c feat/roda-debate
   ```
3. Para a #21: aguardar/integrar #13, #19 e #20 antes de começar.
4. Recomendação de sequência:
   - fechar e mergear a #17;
   - rebasear a #13 em cima da `main` atualizada;
   - mergear #19 e #20;
   - criar a branch `feat/tela-debate` a partir da `main` atualizada.

---

### Etapa 1 — #17: fazer o debate começar em outro suspeito a cada dia

**Arquivos principais:** `jogo.h`, `jogo.c`.

#### 1. Adicionar o próximo ponto de partida ao estado

Em `jogo.h`, adicionar ao `struct Jogo`:

```c
int proximo_inicio_debate; // id do suspeito que deve abrir o próximo debate
```

#### 2. Inicializar na criação da partida

Em `iniciar_jogo_com_semente` (`jogo.c`):

```c
jogo->proximo_inicio_debate = 0; // dia 1 continua começando no Gerente
```

#### 3. Criar auxiliares em `jogo.c`

- `no_por_id(jogo, id)` — percorre a lista circular e devolve o nó com o id.
- `primeiro_vivo_a_partir(jogo, no)` — devolve o primeiro nó com `situacao == VIVO`, dando a volta se necessário.

Exemplo de implementação:

```c
static NoListaDuplaCircular *no_por_id(const Jogo *jogo, int id) {
    if (jogo == NULL || jogo->suspeitos == NULL ||
        jogo->suspeitos->inicio == NULL) {
        return NULL;
    }

    NoListaDuplaCircular *atual = jogo->suspeitos->inicio;
    do {
        if (atual->id == id) {
            return atual;
        }
        atual = atual->prox;
    } while (atual != jogo->suspeitos->inicio);

    return NULL;
}

static NoListaDuplaCircular *primeiro_vivo_a_partir(
    const Jogo *jogo, NoListaDuplaCircular *comeco) {
    if (jogo == NULL || comeco == NULL) {
        return NULL;
    }

    NoListaDuplaCircular *atual = comeco;
    do {
        Suspeito *suspeito = (Suspeito *)atual->dado;
        if (suspeito != NULL && suspeito->situacao == VIVO) {
            return atual;
        }
        atual = atual->prox;
    } while (atual != comeco);

    return NULL;
}
```

#### 4. Reescrever `iniciar_debate`

```c
void iniciar_debate(Jogo *jogo) {
    if (jogo == NULL || jogo->suspeitos == NULL) {
        return;
    }

    jogo->interrogados = 0;
    jogo->turno_atual = NULL;

    NoListaDuplaCircular *candidato =
        no_por_id(jogo, jogo->proximo_inicio_debate);

    jogo->turno_atual = primeiro_vivo_a_partir(
        jogo,
        candidato != NULL ? candidato : jogo->suspeitos->inicio
    );

    if (jogo->turno_atual != NULL) {
        Suspeito *primeiro = (Suspeito *)jogo->turno_atual->dado;
        jogo->proximo_inicio_debate =
            (primeiro->id + 1) % TOTAL_SUSPEITOS;
    }
}
```

#### 5. Manter `avancar_turno`

O `avancar_turno` atual já:

- pula quem não está `VIVO`;
- pula quem já está no bitmap `interrogados`;
- para quando todos os vivos falaram.

A única mudança é que o **primeiro nó do dia** deixa de ser sempre `suspeitos->inicio`.

#### 6. Robustez contra chamadas repetidas

`iniciar_debate` é chamada uma vez por dia por `avancar_fase_jogo`. Se houver risco de chamada repetida no mesmo dia, guardar também o dia em que o debate foi iniciado:

```c
int dia_debate_iniciado;
```

e só atualizar `proximo_inicio_debate` quando o dia mudar. Caso contrário, documentar que a função deve ser chamada uma vez por dia.

#### 7. Registrar a decisão

Adicionar comentário em `jogo.h`, perto de `iniciar_debate`, explicando:

- mortos e presos permanecem na lista;
- o debate os ignora pela `situacao`;
- a lista representa o elenco completo, não apenas a roda.

Também registrar a decisão na descrição do PR/issue.

---

### Etapa 2 — #17: testes

Criar `test/teste_roda_debate.c` para evitar conflito com o `teste_jogo.c`, que a #13 já altera.

#### Casos de teste

1. **Dia 2 começa em outro suspeito**
   - Iniciar partida.
   - Avançar manhã → debate, guardar o primeiro falante.
   - Avançar debate → julgamento → noite → manhã do dia 2 → debate.
   - Verificar que `suspeito_do_turno` do dia 2 tem id diferente do dia 1.

2. **Pula morto e preso**
   - Marcar um suspeito como `MORTO` e outro como `PRESO`.
   - Chamar `iniciar_debate`.
   - Percorrer a volta completa e verificar que nenhum dos dois recebe turno.
   - Verificar que todos os vivos falam exatamente uma vez.

3. **Até sobrar 1 vivo**
   - Marcar todos menos um como `MORTO`/`PRESO`.
   - Verificar que o único vivo recebe o turno.
   - Verificar que `perguntar_sobre` funciona.
   - Verificar que `debate_encerrado` fica verdadeiro depois.

4. **Cobertura de remoção da lista**
   - Como a decisão é manter e pular, os testes de remoção do meio/início/sobrar 1 já existem em `test/teste_lista_circular.c`.
   - Nenhuma mudança em `contar_vivos` é necessária.

#### Registro do teste no build

Em `CMakeLists.txt`:

```cmake
ultimo_turno_adicionar_teste(teste_roda_debate
    test/teste_roda_debate.c
    jogo.c
    dia.c
    ${ULTIMO_TURNO_ESTRUTURAS}
)
```

No `Makefile`, adicionar:

- `TESTE_RODA_DEBATE_SRC`;
- `TESTE_RODA_DEBATE_IMPL` (usar `TESTE_JOGO_IMPL`);
- entrada em `TESTE_BINS`;
- regra de compilação.

#### Comandos de validação

```sh
make test
ctest --test-dir build-cmake --output-on-failure
make valgrind
```

---

### Etapa 3 — Integrar #13, #19 e #20

Quando essas issues entrarem na `main`:

1. Rebasear a #13 (`origin/feat/jogo-dedutivel`) em cima da `main` atualizada.
2. Resolver conflitos esperados em:
   - `jogo.h` / `jogo.c` — campo `proximo_inicio_debate` + agenda/períodos;
   - `test/teste_jogo.c` — a #13 já reescreve esse arquivo;
   - `CMakeLists.txt` / `Makefile` — novos testes (`teste_debate.c`, etc.).
3. Confirmar que `perguntar_sobre` agora recebe `enum Periodo`.
4. Confirmar que a #19 publicou `interface/ui.c` e `interface/ui.h`.
5. Confirmar que a #20 publicou o fluxo de telas e o estado de tela atual.
6. Criar a branch da UI:
   ```sh
   git switch main
   git pull
   git switch -c feat/tela-debate
   ```

---

### Etapa 4 — Infraestrutura da tela de debate

**Arquivos previstos:**

- `interface/ui.h` / `interface/ui.c` — reutilizar os helpers da #19.
- `interface/tela_debate.h` / `interface/tela_debate.c` — tela em si.
- `main.c` — estado de tela e transições da #20.
- `CMakeLists.txt` / `Makefile` — incluir os arquivos no alvo gráfico.

#### Estado sugerido da tela

```c
typedef struct {
    int id_alvo_selecionado;
    enum Periodo periodo_selecionado; // se a #13 estiver presente
    int id_depoimento_atual;
    float alerta_contradicao;         // segundos restantes
    int contradicoes_antes;
} TelaDebate;
```

#### Funções sugeridas

```c
void tela_debate_iniciar(TelaDebate *t, Jogo *jogo);
AcaoDebate tela_debate_atualizar(TelaDebate *t, Jogo *jogo);
void tela_debate_desenhar(const TelaDebate *t, const Jogo *jogo, const UI *ui);
```

`AcaoDebate` pode ser:

```c
typedef enum {
    DEBATE_CONTINUA,
    DEBATE_ABRIR_CADERNO,
    DEBATE_ENCERRAR
} AcaoDebate;
```

#### Layout sugerido para 960×540

```text
+--------------------------------------------------------------+
| Dia 2/4 | Debate | Credibilidade: 85                          |
+-------------------------------+------------------------------+
| Vez de: Almeida (Gerente)     | Período: [Manhã][Tarde][Noite]|
|                               | Alvo:                        |
| "Eu estava na Gerência..."    |  ( ) Almeida                 |
| Local: Gerência               |  (x) Bastos                  |
|                               |  ( ) Costa                   |
| Reações:                      |  ...                         |
|  ✓ Capataz   ✗ Vigia          | [Interrogar]                 |
|  – Enfermeira                 | [Abrir caderno]              |
|                               | [Encerrar debate]            |
+-------------------------------+------------------------------+
| ⚠ Nova contradição registrada (2)                            |
+--------------------------------------------------------------+
```

O “⚠” deve ser **desenhado com formas**, não como caractere da fonte.

---

### Etapa 5 — Ações da tela

#### 1. Suspeito da vez

- Chamar `suspeito_do_turno(&jogo)` a cada quadro.
- Se devolver `NULL`, mostrar “Todos os suspeitos vivos já falaram”.
- Desabilitar “Interrogar”.

#### 2. Lista dos vivos

- Percorrer `jogo.suspeitos`.
- Mostrar apenas `situacao == VIVO`.
- Incluir o próprio falante, como no terminal, para ouvir o álibi dele.
- Destacar o alvo selecionado.

#### 3. Período (se #13)

- Três botões: Manhã, Tarde e Noite.
- Default sugerido: Noite, por ser o período do crime.
- “Interrogar” só habilita com alvo escolhido.

#### 4. Interrogar

```c
int antes = jogo->caderno->contradicoes.tamanho;
int id = perguntar_sobre(&jogo, alvo, periodo); // ou sem periodo na main
int depois = jogo->caderno->contradicoes.tamanho;

if (id > 0) {
    t->id_depoimento_atual = id;
    if (depois > antes) {
        t->alerta_contradicao = 3.0f;
    }
}
```

Depois:

- buscar o depoimento com `buscar_depoimento_por_id`;
- mostrar falante, texto, local, dia e período.

#### 5. Reações

- Iterar `jogo->caderno->reacoes`.
- Filtrar por `id_depoimento`.
- Para cada reação, mostrar `quem_reagiu` + ícone:
  - `CONCORDA`: verde/check;
  - `DISCORDA`: vermelho/X;
  - `NEUTRO`: cinza/traço.
- Se não houver reações, mostrar “Sem reações”.

#### 6. Aviso de nova contradição

- Não rodar filtro por quadro.
- Usar o delta de `contradicoes.tamanho` no momento da pergunta.
- Diminuir `alerta_contradicao` com `GetFrameTime()`.
- Desenhar um banner enquanto `alerta_contradicao > 0`.

#### 7. Botões “Abrir caderno” e “Encerrar debate”

- **“Encerrar debate”**:
  - sempre habilitado;
  - chama `avancar_fase_jogo` e vai para o julgamento;
  - `debate_encerrado(&jogo)` indica o fim natural da volta e desabilita “Interrogar”;
  - opcionalmente, pedir confirmação se ainda houver turnos.
- **“Abrir caderno”**:
  - abrir uma tela/overlay “Caderno em construção” com botão de voltar;
  - quando a #22 existir, substituir pelo caderno real.

---

### Etapa 6 — Build e integração

#### CMake

Adicionar `interface/ui.c` e `interface/tela_debate.c` ao alvo `jogo` (não ao `jogo-terminal`):

```cmake
add_executable(jogo
    main.c
    jogo.c
    dia.c
    interface/ui.c
    interface/tela_debate.c
    ${ULTIMO_TURNO_ESTRUTURAS}
)
```

#### Makefile

- Incluir `$(wildcard interface/*.c)` em `JOGO_SRCS`.
- Incluir `interface/*.h` em `CABECALHOS`.
- Não incluir a UI em `TERMINAL_SRCS`, para o terminal continuar sem raylib.

#### Assets

- Se houver fonte `.ttf`, adicionar em `assets/`.
- O CMake já copia a pasta `assets/` para junto do executável gráfico.
- O Makefile precisa rodar a partir da raiz ou receber o caminho dos assets.

#### Comandos

```sh
cmake -S . -B build-cmake -DULTIMO_TURNO_BUILD_JOGO=ON
cmake --build build-cmake --target jogo
make jogo
make test
ctest --test-dir build-cmake --output-on-failure
```

---

### Etapa 7 — Validação manual e documentação

#### Checklist manual

- [ ] Menu → escolha da detetive → introdução → manhã → debate.
- [ ] O suspeito da vez aparece corretamente.
- [ ] A lista mostra só os vivos.
- [ ] Interrogar mostra depoimento, local, período e reações com ícones.
- [ ] Aviso de nova contradição aparece e some depois de ~3 s.
- [ ] “Abrir caderno” abre e volta sem perder o estado do debate.
- [ ] “Encerrar debate” leva ao julgamento.
- [ ] No dia 2, o debate começa em outro suspeito.
- [ ] Suspeito morto/preso não aparece/recebe turno.
- [ ] Acentos aparecem corretamente.
- [ ] Textos longos quebram linha dentro do painel.
- [ ] Não há crash ao interrogar o último suspeito ou ao encerrar sem interrogar todos.

#### Documentação

- Registrar a decisão de manter mortos/presos na lista no comentário do código e no PR.
- Se o fluxo gráfico ficar jogável, atualizar o “Como jogar” do README.
- Caso contrário, deixar claro que o terminal continua sendo a versão completa.

---

## 5. Critérios de aceite

### #17

- [ ] O dia 1 pode continuar começando no Gerente.
- [ ] O dia 2 começa em outro suspeito vivo.
- [ ] O ponto de partida avança mesmo se o próximo suspeito morreu ou foi preso.
- [ ] Todos os vivos falam exatamente uma vez por debate.
- [ ] Mortos e presos não recebem turno.
- [ ] A decisão sobre remover ou pular está documentada.
- [ ] `make test`, `ctest` e valgrind/ASan sem erros.

### #21

- [ ] Tela alcançável pelo fluxo do jogo.
- [ ] Mostra `suspeito_do_turno`.
- [ ] Lista os vivos para escolher o alvo.
- [ ] Se #13 estiver presente, permite escolher o período.
- [ ] Chama `perguntar_sobre` e exibe depoimento + reações com ícones.
- [ ] “Abrir caderno” e “Encerrar debate” funcionam.
- [ ] Usa `debate_encerrado`.
- [ ] Avisa nova contradição comparando o tamanho da seção antes/depois.
- [ ] Compila no CMake e no Makefile, sem warnings novos.

---

## 6. Riscos

1. **Conflito com a #13**
   - Ela muda a assinatura de `perguntar_sobre`, o struct `Jogo` e os testes.
   - Mitigação: fazer #17 antes e rebasear #13 em cima dela.

2. **Dependência de #19/#20**
   - Sem helpers e fluxo de telas, a #21 vira uma implementação maior do que parece.
   - Mitigação: aguardar as issues ou coordenar uma fatia mínima.

3. **Fonte sem acentos**
   - Hoje não há `.ttf` em `assets/`.
   - Mitigação: testar a fonte padrão da raylib e, se necessário, adicionar uma fonte livre.

4. **Remover mortos da lista**
   - É mais trabalhoso e arriscado do que parece.
   - Mitigação: manter e pular, conforme decisão registrada.

5. **UI sem testes automatizados**
   - A validação será manual; os testes automatizados cobrem o motor.
   - Mitigação: checklist manual e compilação no CI gráfico.

---

## 7. Backlog resumido

### Agora

- [ ] Criar `feat/roda-debate` a partir da `main`.
- [ ] Adicionar `proximo_inicio_debate` ao `Jogo`.
- [ ] Implementar `no_por_id` e `primeiro_vivo_a_partir`.
- [ ] Alterar `iniciar_debate`.
- [ ] Documentar a decisão de pular mortos/presos.
- [ ] Criar `test/teste_roda_debate.c`.
- [ ] Registrar o teste no CMake e no Makefile.
- [ ] Rodar `make test`, `ctest` e valgrind/ASan.
- [ ] Abrir PR da #17.

### Depois

- [ ] Integrar #13, #19 e #20 na `main`.
- [ ] Criar `feat/tela-debate`.
- [ ] Implementar `interface/ui` (se ainda não existir).
- [ ] Implementar `interface/tela_debate`.
- [ ] Integrar com o fluxo de telas da #20.
- [ ] Implementar o placeholder do caderno.
- [ ] Implementar o alerta de contradição.
- [ ] Validar manualmente a tela.
- [ ] Abrir PR da #21.

---

## 8. Referências rápidas

### Arquivos do motor

- `jogo.h` — estado do jogo e assinaturas públicas.
- `jogo.c` — ciclo de vida, fases, debate, julgamento.
- `dia.h` / `dia.c` — dias e fases.
- `estruturas/listas/lista-dupla-circular.c` — lista circular.
- `estruturas/entidades/caderno.c` — depoimentos, pistas, reações e contradições.

### Arquivos de interface

- `main.c` — menu atual em raylib.
- `jogo_terminal.c` — referência de comportamento do terminal.
- `CMakeLists.txt` — alvos `jogo` e `jogo-terminal`.
- `Makefile` — alvos `jogo`, `terminal`, `test` e `valgrind`.

### Referências das issues

- #13 — modelo de verdade e mentiras.
- #17 — roda do debate (este documento, primeira parte).
- #19 — helpers de UI raylib.
- #20 — telas iniciais e fluxo do dia.
- #21 — tela do debate (este documento, segunda parte).
- #22 — tela do caderno.
- #23 — telas de julgamento, noite e fim.
