cmake_minimum_required(VERSION 3.22)

project(ultimo_turno LANGUAGES C)

# ---------------------------------------------------------------------------
# Configuracao padrao
# ---------------------------------------------------------------------------
# Build Release por padrao em geradores de configuracao unica (Linux/macOS).
# Em geradores multi-config (Visual Studio/Xcode) NAO definimos a configuracao
# aqui: o Visual Studio rejeita CMAKE_DEFAULT_BUILD_TYPE na geracao.
# Nesses casos o CI (e o usuario) passa --config Release explicitamente.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de build" FORCE)
endif()

# C11 com extensoes GNU (gnu11).
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

option(ULTIMO_TURNO_BUILD_JOGO "Compila o jogo grafico com a raylib" ON)

# ---------------------------------------------------------------------------
# Fontes compartilhadas
# ---------------------------------------------------------------------------
set(ULTIMO_TURNO_ESTRUTURAS
    estruturas/entidades/aleatorio.c
    estruturas/entidades/caderno.c
    estruturas/entidades/locais.c
    estruturas/entidades/personagens.c
    estruturas/entidades/suspeito.c
    estruturas/entidades/texto.c
    estruturas/listas/lista-dupla.c
    estruturas/listas/lista-dupla-circular.c
    estruturas/listas/lista-simples.c
)

set(ULTIMO_TURNO_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/estruturas
    ${CMAKE_CURRENT_SOURCE_DIR}/interface
)

set(ULTIMO_TURNO_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")

function(ultimo_turno_configurar_alvo alvo)
    target_include_directories(${alvo} PRIVATE ${ULTIMO_TURNO_INCLUDES})

    if(MSVC)
        # /utf-8 garante que os literais acentuados sejam compilados como UTF-8.
        # /std:c11 liga os inicializadores designados usados em jogo.c.
        target_compile_options(${alvo} PRIVATE /W4 /utf-8 /std:c11)
    else()
        target_compile_options(${alvo} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Alvo 1: versao de terminal (sem raylib)
# ---------------------------------------------------------------------------
add_executable(jogo-terminal
    terminal/main.c
    jogo.c
    jogo_terminal.c
    dia.c
    ${ULTIMO_TURNO_ESTRUTURAS}
)
ultimo_turno_configurar_alvo(jogo-terminal)

# ---------------------------------------------------------------------------
# Alvo 2: jogo grafico (com raylib)
# O submodulo raylib esta fixado na tag 6.2.
# ---------------------------------------------------------------------------
if(ULTIMO_TURNO_BUILD_JOGO)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/raylib/CMakeLists.txt")
        message(FATAL_ERROR
            "raylib nao encontrada em raylib/. "
            "Rode: git submodule update --init --recursive")
    endif()

    # Nao compilar os exemplos da raylib junto com o jogo.
    set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)

    add_subdirectory(raylib)

    add_executable(jogo
        main.c
        interface/ui.c
        interface/telas/tela_caderno.c
        interface/recursos.c
        interface/telas.c
        interface/telas/tela_debate.c
        jogo.c
        dia.c
        ${ULTIMO_TURNO_ESTRUTURAS}
    )
    ultimo_turno_configurar_alvo(jogo)

    target_link_libraries(jogo PRIVATE raylib)
    if(UNIX)
        # libm e necessaria para as funcoes matematicas usadas pelo jogo.
        target_link_libraries(jogo PRIVATE m)
    endif()

    # Copia assets/ (logo, fonte etc.) para junto do executavel grafico.
    if(EXISTS "${ULTIMO_TURNO_ASSETS_DIR}")
        add_custom_command(TARGET jogo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${ULTIMO_TURNO_ASSETS_DIR}"
                "$<TARGET_FILE_DIR:jogo>/assets"
            COMMENT "Copiando assets para junto do executavel grafico"
            VERBATIM
        )
    else()
        message(STATUS "Pasta assets/ ausente; o jogo grafico sera gerado sem assets.")
    endif()
endif()

# ---------------------------------------------------------------------------
# Testes
# ---------------------------------------------------------------------------
enable_testing()

function(ultimo_turno_adicionar_teste nome)
    add_executable(${nome} ${ARGN})
    ultimo_turno_configurar_alvo(${nome})

    # O build padrao e Release, que define NDEBUG e desliga os assert().
    # Nos testes, mantemos os assert() ativos.
    if(MSVC)
        target_compile_options(${nome} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${nome} PRIVATE -UNDEBUG)
    endif()

    add_test(NAME ${nome} COMMAND ${nome})
endfunction()

ultimo_turno_adicionar_teste(teste_lista_simples
    test/teste_lista_simples.c
    estruturas/listas/lista-simples.c
)

ultimo_turno_adicionar_teste(teste_lista_dupla
    test/teste_lista_dupla.c
    estruturas/listas/lista-dupla.c
)

ultimo_turno_adicionar_teste(teste_lista_circular
    test/teste_lista_circular.c
    estruturas/listas/lista-dupla-circular.c
)

ultimo_turno_adicionar_teste(teste_caderno
    test/teste_caderno.c
    estruturas/entidades/caderno.c
    estruturas/entidades/texto.c
    estruturas/listas/lista-dupla.c
    estruturas/listas/lista-simples.c
)

ultimo_turno_adicionar_teste(teste_suspeito
    test/teste_suspeito.c
    estruturas/entidades/suspeito.c
    estruturas/entidades/texto.c
    estruturas/listas/lista-dupla-circular.c
)

ultimo_turno_adicionar_teste(teste_texto
    test/teste_texto.c
    estruturas/entidades/texto.c
)

ultimo_turno_adicionar_teste(teste_aleatorio
    test/teste_aleatorio.c
    estruturas/entidades/aleatorio.c
)

ultimo_turno_adicionar_teste(teste_dia
    test/teste_dia.c
    dia.c
)

ultimo_turno_adicionar_teste(teste_jogo
    test/teste_jogo.c
    jogo.c
    dia.c
    ${ULTIMO_TURNO_ESTRUTURAS}
)

ultimo_turno_adicionar_teste(teste_roda_debate
    test/teste_roda_debate.c
    jogo.c
    dia.c
    ${ULTIMO_TURNO_ESTRUTURAS}
)

ultimo_turno_adicionar_teste(teste_locais
    test/teste_locais.c
    estruturas/entidades/locais.c
)

ultimo_turno_adicionar_teste(teste_agenda
    test/teste_agenda.c
    jogo.c
    dia.c
    ${ULTIMO_TURNO_ESTRUTURAS}
)

ultimo_turno_adicionar_teste(teste_debate
    test/teste_debate.c
    jogo.c
    dia.c
    ${ULTIMO_TURNO_ESTRUTURAS}
)

# O teste da interface usa a raylib para ler o .ttf e conferir os glifos, mas
# não abre janela; por isso roda mesmo sem display, desde que a raylib exista.
# O diretório de trabalho é a raiz do repositório para achar assets/fontes/.
if(ULTIMO_TURNO_BUILD_JOGO)
    ultimo_turno_adicionar_teste(teste_ui
        test/teste_ui.c
        interface/ui.c
    )
    target_link_libraries(teste_ui PRIVATE raylib)
    if(UNIX)
        target_link_libraries(teste_ui PRIVATE m)
    endif()
    set_tests_properties(teste_ui PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()


