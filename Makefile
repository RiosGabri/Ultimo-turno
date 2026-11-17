CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -g
BUILD := build

# ---------------------------------------------------------------------------
# Estrutura de pastas do projeto
# ---------------------------------------------------------------------------
ESTRUTURAS_DIR := estruturas
TEST_DIR := test

INCLUDES := -I. -I$(ESTRUTURAS_DIR) -Iinterface

# ---------------------------------------------------------------------------
# Dependências externas (git submodules)
# ---------------------------------------------------------------------------
RAYLIB_DIR ?= raylib
RAYLIB_SRC := $(RAYLIB_DIR)/src
RAYLIB_LIB := $(RAYLIB_SRC)/libraylib.a

RAYLIB_CFLAGS := -I$(RAYLIB_SRC)
# Bibliotecas do sistema necessárias para linkar a raylib estática no Linux.
RAYLIB_LIBS := -L$(RAYLIB_SRC) -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# ---------------------------------------------------------------------------
# Fontes
# ---------------------------------------------------------------------------
# Jogo: arquivos na raiz, em estruturas/listas/ e estruturas/entidades/
# (a pasta test/ fica de fora). As funções das listas têm prefixos diferentes,
# então as três implementações podem ser linkadas juntas. Se quiser linkar
# apenas uma, por exemplo:
#   make JOGO_ESTRUTURAS=estruturas/listas/lista-simples.c
JOGO_ESTRUTURAS ?= $(wildcard $(ESTRUTURAS_DIR)/listas/*.c) \
                   $(wildcard $(ESTRUTURAS_DIR)/entidades/*.c)
JOGO_SRCS := $(wildcard *.c) interface/ui.c $(JOGO_ESTRUTURAS)
CABECALHOS := $(wildcard *.h) $(wildcard interface/*.h) \
              $(wildcard $(ESTRUTURAS_DIR)/*.h) \
              $(wildcard $(ESTRUTURAS_DIR)/*/*.h)

# Versão de terminal: não depende da raylib nem do main gráfico.
TERMINAL_SRCS := terminal/main.c jogo.c jogo_terminal.c dia.c $(JOGO_ESTRUTURAS)

# Cada teste linka apenas a implementação que está testando, porque as
# listas compartilham os mesmos nomes de funções.
TESTE_LISTA_SIMPLES_SRC := $(TEST_DIR)/teste_lista_simples.c
TESTE_LISTA_DUPLA_SRC := $(TEST_DIR)/teste_lista_dupla.c
TESTE_LISTA_CIRCULAR_SRC := $(TEST_DIR)/teste_lista_circular.c
TESTE_CADERNO_SRC := $(TEST_DIR)/teste_caderno.c
TESTE_SUSPEITO_SRC := $(TEST_DIR)/teste_suspeito.c
TESTE_TEXTO_SRC := $(TEST_DIR)/teste_texto.c
TESTE_DIA_SRC := $(TEST_DIR)/teste_dia.c
TESTE_JOGO_SRC := $(TEST_DIR)/teste_jogo.c
TESTE_UI_SRC := $(TEST_DIR)/teste_ui.c

TESTE_LISTA_SIMPLES_IMPL := $(ESTRUTURAS_DIR)/listas/lista-simples.c
TESTE_LISTA_DUPLA_IMPL := $(ESTRUTURAS_DIR)/listas/lista-dupla.c
TESTE_LISTA_CIRCULAR_IMPL := $(ESTRUTURAS_DIR)/listas/lista-dupla-circular.c
# O caderno usa a lista dupla (seções) e a lista simples (resultado do filtro).
TESTE_CADERNO_IMPL := $(ESTRUTURAS_DIR)/entidades/caderno.c \
                      $(ESTRUTURAS_DIR)/entidades/texto.c \
                      $(ESTRUTURAS_DIR)/listas/lista-simples.c \
                      $(ESTRUTURAS_DIR)/listas/lista-dupla.c
# Suspeitos ficam na lista dupla circular.
TESTE_SUSPEITO_IMPL := $(ESTRUTURAS_DIR)/entidades/suspeito.c \
                       $(ESTRUTURAS_DIR)/entidades/texto.c \
                       $(ESTRUTURAS_DIR)/listas/lista-dupla-circular.c
# Cópia portátil de strings (substitui strdup).
TESTE_TEXTO_IMPL := $(ESTRUTURAS_DIR)/entidades/texto.c
# Controle de dias e fases fica no módulo dia.
TESTE_DIA_IMPL := dia.c
# O motor do jogo usa dia, caderno, personagens e as três listas.
TESTE_JOGO_IMPL := jogo.c dia.c $(wildcard $(ESTRUTURAS_DIR)/*/*.c)
# O teste da interface usa raylib para ler o .ttf e conferir os glifos, mas
# não abre janela.
TESTE_UI_IMPL := interface/ui.c $(RAYLIB_LIB)

# Só entram no alvo test os arquivos de teste existentes e não vazios.
# Assim o Makefile continua válido enquanto os testes novos são escritos.
TESTE_BINS :=
ifneq ($(strip $(shell test -s $(TESTE_LISTA_SIMPLES_SRC) && echo 1)),)
TESTE_BINS += $(BUILD)/teste_lista_simples
endif
ifneq ($(strip $(shell test -s $(TESTE_LISTA_DUPLA_SRC) && echo 1)),)
TESTE_BINS += $(BUILD)/teste_lista_dupla
endif
ifneq ($(strip $(shell test -s $(TESTE_LISTA_CIRCULAR_SRC) && echo 1)),)
TESTE_BINS += $(BUILD)/teste_lista_circular
endif
ifneq ($(strip $(shell test -s $(TESTE_CADERNO_SRC) && echo 1)),)
TESTE_BINS += $(BUILD)/teste_caderno
endif
ifneq ($(strip $(shell test -s $(TESTE_SUSPEITO_SRC) && echo 1)),)
TESTE_BINS += $(BUILD)/teste_suspeito
endif
ifneq ($(strip $(shell test -s $(TESTE_TEXTO_SRC) && echo 1)),)
TESTE_BINS += $(BUILD)/teste_texto
endif
ifneq ($(strip $(shell test -s $(TESTE_DIA_SRC) && echo 1)),)
TESTE_BINS += $(BUILD)/teste_dia
endif
ifneq ($(strip $(shell test -s $(TESTE_JOGO_SRC) && echo 1)),)
TESTE_BINS += $(BUILD)/teste_jogo
endif
# O teste de interface precisa da raylib. Para não obrigar quem só quer testar
# as estruturas a compilá-la, ele entra em `make test` apenas quando a
# biblioteca já existe; `make test-ui` compila a raylib se necessário.
ifneq ($(strip $(shell test -s $(TESTE_UI_SRC) && echo 1)),)
ifneq ($(wildcard $(RAYLIB_LIB)),)
TESTE_BINS += $(BUILD)/teste_ui
endif
endif

.PHONY: all jogo terminal run test test-ui valgrind submodules raylib clean distclean FORCE

all: jogo

jogo: $(BUILD)/jogo

terminal: $(BUILD)/jogo-terminal

run: $(BUILD)/jogo-terminal
	./$(BUILD)/jogo-terminal

# ---------------------------------------------------------------------------
# raylib (submodule)
# ---------------------------------------------------------------------------
submodules:
	git submodule update --init --recursive

# Sempre chama o make da raylib: ele próprio decide se precisa recompilar.
# Assim, mudanças nos fontes do submodule também são detectadas.
$(RAYLIB_LIB): FORCE
	@if [ ! -f "$(RAYLIB_SRC)/Makefile" ]; then \
		echo "Erro: raylib não encontrada em $(RAYLIB_SRC)."; \
		echo "Rode: make submodules"; \
		exit 1; \
	fi
	$(MAKE) -C $(RAYLIB_SRC) PLATFORM=PLATFORM_DESKTOP

raylib: $(RAYLIB_LIB)

# ---------------------------------------------------------------------------
# Jogo
# ---------------------------------------------------------------------------
$(BUILD)/jogo: $(JOGO_SRCS) $(CABECALHOS) $(RAYLIB_LIB)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) $(RAYLIB_CFLAGS) $(JOGO_SRCS) -o $@ $(RAYLIB_LIBS)

# Versão de terminal: compila sem raylib, para testar as regras do jogo.
$(BUILD)/jogo-terminal: $(TERMINAL_SRCS) $(CABECALHOS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) $(TERMINAL_SRCS) -o $@

# ---------------------------------------------------------------------------
# Testes das estruturas
# ---------------------------------------------------------------------------
$(BUILD)/teste_lista_simples: $(TESTE_LISTA_SIMPLES_SRC) $(TESTE_LISTA_SIMPLES_IMPL) \
		$(ESTRUTURAS_DIR)/listas/lista-simples.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(TESTE_LISTA_SIMPLES_IMPL) -o $@

$(BUILD)/teste_lista_dupla: $(TESTE_LISTA_DUPLA_SRC) $(TESTE_LISTA_DUPLA_IMPL) \
		$(ESTRUTURAS_DIR)/listas/lista-dupla.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(TESTE_LISTA_DUPLA_IMPL) -o $@

$(BUILD)/teste_lista_circular: $(TESTE_LISTA_CIRCULAR_SRC) $(TESTE_LISTA_CIRCULAR_IMPL) \
		$(ESTRUTURAS_DIR)/listas/lista-dupla-circular.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(TESTE_LISTA_CIRCULAR_IMPL) -o $@

$(BUILD)/teste_caderno: $(TESTE_CADERNO_SRC) $(TESTE_CADERNO_IMPL) \
		$(ESTRUTURAS_DIR)/entidades/caderno.h \
		$(ESTRUTURAS_DIR)/entidades/texto.h \
		$(ESTRUTURAS_DIR)/listas/lista-simples.h \
		$(ESTRUTURAS_DIR)/listas/lista-dupla.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(TESTE_CADERNO_IMPL) -o $@

$(BUILD)/teste_suspeito: $(TESTE_SUSPEITO_SRC) $(TESTE_SUSPEITO_IMPL) \
		$(ESTRUTURAS_DIR)/entidades/suspeito.h \
		$(ESTRUTURAS_DIR)/entidades/texto.h \
		$(ESTRUTURAS_DIR)/listas/lista-dupla-circular.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(TESTE_SUSPEITO_IMPL) -o $@

$(BUILD)/teste_texto: $(TESTE_TEXTO_SRC) $(TESTE_TEXTO_IMPL) \
		$(ESTRUTURAS_DIR)/entidades/texto.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(TESTE_TEXTO_IMPL) -o $@

$(BUILD)/teste_dia: $(TESTE_DIA_SRC) $(TESTE_DIA_IMPL) dia.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(TESTE_DIA_IMPL) -o $@

$(BUILD)/teste_jogo: $(TESTE_JOGO_SRC) $(TESTE_JOGO_IMPL) $(CABECALHOS)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) $(TESTE_JOGO_SRC) $(TESTE_JOGO_IMPL) -o $@

$(BUILD)/teste_ui: $(TESTE_UI_SRC) $(TESTE_UI_IMPL) interface/ui.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDES) $(RAYLIB_CFLAGS) $(TESTE_UI_SRC) interface/ui.c -o $@ $(RAYLIB_LIBS)

test: $(TESTE_BINS)
	@status=0; \
	for teste in $(TESTE_BINS); do \
		echo "== $$teste"; \
		./$$teste || status=1; \
	done; \
	exit $$status

# Teste da fonte/.ttf e dos auxiliares de interface. Compila a raylib se ela
# ainda não estiver disponível.
test-ui: $(BUILD)/teste_ui
	./$(BUILD)/teste_ui

valgrind: $(TESTE_BINS)
	@status=0; \
	for teste in $(TESTE_BINS); do \
		echo "== $$teste"; \
		valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./$$teste || status=1; \
	done; \
	exit $$status

# ---------------------------------------------------------------------------
# Limpeza
# ---------------------------------------------------------------------------
clean:
	rm -rf $(BUILD)

# Remove também os artefatos compilados da raylib (o código do submodule é mantido).
distclean: clean
	-$(MAKE) -C $(RAYLIB_SRC) clean

FORCE:
