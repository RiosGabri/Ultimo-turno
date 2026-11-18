# Relatório — 3 partidas completas no terminal

- **Data:** 2026-11-17
- **Base:** `e252535` (`main`)
- **Issue relacionada:** #29 (Integração, partidas de teste e ponto de decisão)
- **Binário:** `build/jogo-terminal` (`make terminal`)
- **Driver:** `play_terminal.py` (pexpect; artefato local, não versionado)

## Partidas

| # | Detetive | Resultado | Assassino | Cúmplice | Dia | Credibilidade | Contradições | Pontos |
|---|----------|-----------|-----------|----------|-----|---------------|--------------|--------|
| 1 | Miss Marple | Vitória | Capataz | Operário | 1/4 | 75 | 8 | 4750 |
| 2 | Hercule Poirot | Derrota por credibilidade | Gerente | Capataz | 3/4 | 0 | 8 | 600 |
| 3 | Hercule Poirot | Derrota por tempo | Operário | Vigia noturno | 4/4 | 75 | 8 | 1150 |

Os três pares assassino/cúmplice são diferentes e os dois detetives foram
usados. A pontuação confere com a fórmula: `dias_restantes×200 +
credibilidade×10 + contradições×50 + 3000` (vitória).

Logs locais das partidas: `partida1-win.log`, `partida2-cred.log` e
`partida3-time.log` (em `/tmp/opencode/`, não versionados).

### Como os papéis foram identificados

Em cada debate, todos os suspeitos vivos foram interrogados sobre o mesmo alvo
no período da noite. Inocentes dizem a verdade; assassino e cúmplice mentem.
O par que reporta o local minoritário é o par mentiroso.

- Partida 1: prendeu um membro do par no dia 1 (era o assassino) e venceu.
- Partida 2: prendeu três inocentes de propósito (Enfermeira foi a terceira) até
  zerar a credibilidade.
- Partida 3: não prendeu ninguém até o prazo acabar.

## Achados

Cada achado abaixo corresponde a um rascunho de issue em
`/tmp/opencode/issues/` (não versionado). As issues ainda **não foram
publicadas**.

### 1. Revelação do cúmplice usa a noite errada (e no dia 1 não houve crime)

`prender_suspeito` → `preparar_revelacao_cumplice` (`jogo.c:619-628`) usa
`local_verdadeiro_do_suspeito`, que resolve para a noite do **dia do
julgamento**, não para a noite do último crime.

Evidência (harness com semente fixa):

```text
semente 7, julgamento do dia 1
assassino=Operário cumplice=Gerente
local_assassino_noite_1=Sala de máquinas
revelacao_dia1=Revelação de Gerente: o local do assassino na noite do crime era Sala de máquinas.
ultima_vitima=-1 (NENHUM=-1)
```

```text
semente 2024, julgamento do dia 3
local_noite_2 (crime anterior)=Gerência
local_noite_3 (noite atual)   =Sala de máquinas
revelacao=Revelação de Operário: o local do assassino na noite do crime era Sala de máquinas.
revelacao_cita_noite_2=0 revelacao_cita_noite_3=1
```

Impacto: a pista de revelação entra errada no caderno e pode apontar para uma
noite futura ou para uma noite sem crime. O teste `test/teste_jogo.c:447-459`
codifica o comportamento atual ("pista de revelação é do dia atual").

### 2. Regra de ouro comentada / código morto

`gerar_agenda` (`jogo.c:269-270`) tem a chamada comentada:

```c
// regra de ouro: pelo menos um inocente no local do crime
//garantir_testemunha(jogo, d, &estado);
```

`garantir_testemunha` não existe. Medição com 2000 sementes:

```text
noite2_sem_inocente_no_local_do_crime=1045 (52.2%)
noite3_sem_inocente_no_local_do_crime=1279 (64.0%)
```

Pode ser intencional. O achado serve para decidir entre implementar a regra ou
remover o código morto e documentar a decisão.

### 3. "no" fixo no caderno do terminal

`jogo_terminal.c:182-185` usa `"no %s"` em vez de `preposicao_do_local`.
Observado: `Gerente falou sobre Gerente, no Gerência:`. Também há frases
quebradas nas reações (`concordou do depoimento`, `ficou neutro do depoimento`)
em `jogo_terminal.c:199-204`.

### 4. Concordância de gênero

Textos no masculino para a Enfermeira (única personagem feminina):

- `Enfermeira foi encontrado morto na madrugada.` (`jogo_terminal.c:588` e
  `jogo.c:240/245`).
- `Ele não é o assassino` ao prender a Enfermeira (`jogo_terminal.c:566`).
- `o álibi dele` no menu (`jogo_terminal.c:428`).

### 5. README e mensagem de teste dizem credibilidade 100; código usa 75

- `README.md:21`: "começa em 100".
- `test/teste_jogo.c:60`: mensagem "credibilidade começa em 100" (o teste passa
  porque compara com a constante).
- Código: `CREDIBILIDADE_INICIAL 75` (`jogo.h:11`, alterado no PR #40).
- No terminal, o cabeçalho mostra 75 e a pontuação inicial 1350; com 100 seria
  1600.

### 6. EOF imprime "Opção inválida."

`ler_inteiro` (`jogo_terminal.c:17-32`) marca `entrada_encerrada` e retorna
`-1`, mas o menu de debate (`jogo_terminal.c:425-442`) não checa o estado e cai
no `default`:

```text
$ printf '1\n\n\n' | ./build/jogo-terminal
...
> Opção inválida.
exit=0
```

### 7. Partidas no mesmo segundo são idênticas

`iniciar_jogo` usa `(unsigned int)time(NULL)` (`jogo.c:385`). Duas chamadas
seguidas geraram os mesmos papéis e a mesma agenda:

```text
a: assassino=1/cumplice=0 b: assassino=1/cumplice=0
agendas iguais=1
```

Isso afeta o requisito de "assassinos e cúmplices diferentes" em execuções
automatizadas (#29). O motor já tem `iniciar_jogo_com_semente`; falta uma
semente de maior resolução ou uma flag de CLI.

## Verificações

- `make test`: todos os testes passam.
- Valgrind (`--leak-check=full`): 0 erros, 0 leaks.
- ASan/UBSan em uma partida completa: 0 erros.

## Artefatos da sessão (não versionados)

- Logs: `/tmp/opencode/partida1-win.log`, `partida2-cred.log`, `partida3-time.log`
- Driver: `/tmp/opencode/play_terminal.py`
- Harnesses: `/tmp/opencode/audit.c`, `audit2.c`, `seed.c`
- Rascunhos das issues: `/tmp/opencode/issues/01-...` a `07-...`

Nenhuma alteração foi feita no código do jogo; este relatório é o único arquivo
adicionado ao repositório.
