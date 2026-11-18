
#include "tela_julgamento.h"

#include <math.h>
#include <string.h>


static Suspeito *suspeito_disponivel(Jogo *jogo, int id) {
    if (jogo == NULL) {
        return NULL;
    }

    Suspeito *suspeito = buscar_suspeito(jogo, id);

    if (suspeito == NULL || suspeito->situacao != VIVO) {
        return NULL;
    }

    return suspeito;
}

static EntradaBotao ler_entrada(void) {
    return (EntradaBotao){
        .mouse = ui_mouse_canvas(),
        .apertou = IsMouseButtonPressed(MOUSE_BUTTON_LEFT),
        .pressionado = IsMouseButtonDown(MOUSE_BUTTON_LEFT),
        .soltou = IsMouseButtonReleased(MOUSE_BUTTON_LEFT),
        .confirmar = IsKeyPressed(KEY_ENTER) ||
                     IsKeyPressed(KEY_KP_ENTER) ||
                     IsKeyPressed(KEY_SPACE),
    };
}

void tela_julgamento_iniciar(TelaJulgamento *tela, Jogo *jogo,
                            const Recursos *recursos) {
    if (tela == NULL) {
        return;
    }

    memset(tela, 0, sizeof *tela);

    tela->jogo = jogo;
    tela->recursos = recursos;
    tela->etapa = JULGAMENTO_ESCOLHENDO;
    tela->id_selecionado = NENHUM;
    tela->credibilidade_anterior =
        (jogo != NULL) ? jogo->credibilidade : 0;
    tela->botao_focado = -1;

    for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
        tela->botoes_suspeitos[id] = (BotaoUI){
           .area = {
            120.0f + (float)(id % 3) * 350.0f,
            315.0f + (float)(id / 3) * 190.0f,
            320.0f,
            50.0f
        },
            .rotulo = NULL,
            .habilitado = true,
            .primario = false,
        };
    }

    tela->botao_nao_prender = (BotaoUI){
        .area = {450.0f, 590.0f, 380.0f, 50.0f},
        .rotulo = "NAO PRENDER NINGUEM",
        .habilitado = true,
        .primario = false,
    };

    tela->botao_confirmar = (BotaoUI){
        .area = {888.0f, 650.0f, 296.0f, 56.0f},
        .rotulo = "CONFIRMAR",
        .habilitado = true,
        .primario = true,
    };

    tela->botao_cancelar = (BotaoUI){
        .area = {96.0f, 650.0f, 240.0f, 56.0f},
        .rotulo = "CANCELAR",
        .habilitado = true,
        .primario = false,
    };

    tela->botao_continuar = (BotaoUI){
        .area = {888.0f, 650.0f, 296.0f, 56.0f},
        .rotulo = "CONTINUAR",
        .habilitado = true,
        .primario = true,
    };
}



AcaoJulgamento tela_julgamento_atualizar(TelaJulgamento *tela) {
    if (tela == NULL || tela->jogo == NULL) {
        return JULGAMENTO_NENHUMA_ACAO;
    }

    EntradaBotao entrada = ler_entrada();

    if (tela->etapa == JULGAMENTO_ESCOLHENDO) {
        for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
            tela->botoes_suspeitos[id].habilitado =
                suspeito_disponivel(tela->jogo, id) != NULL;

            EstadoBotao estado = ui_atualizar_botao(
                &tela->botoes_suspeitos[id], false, &entrada);

            if (estado == BOTAO_ACIONADO &&
                tela->botoes_suspeitos[id].habilitado) {
                tela->id_selecionado = id;
                tela->etapa = JULGAMENTO_CONFIRMAR;
                tela->botao_focado = 0;
            }
        }

        EstadoBotao estado_nao_prender = ui_atualizar_botao(
            &tela->botao_nao_prender, false, &entrada);

        if (estado_nao_prender == BOTAO_ACIONADO) {
            tela->id_selecionado = NENHUM;
            tela->etapa = JULGAMENTO_CONFIRMAR;
            tela->botao_focado = 0;
        }

    } else if (tela->etapa == JULGAMENTO_CONFIRMAR) {
        EstadoBotao estado_cancelar = ui_atualizar_botao(
            &tela->botao_cancelar, false, &entrada);

        if (estado_cancelar == BOTAO_ACIONADO ||
            IsKeyPressed(KEY_ESCAPE)) {
            tela->etapa = JULGAMENTO_ESCOLHENDO;
            tela->id_selecionado = NENHUM;
            tela->botao_focado = -1;
            return JULGAMENTO_NENHUMA_ACAO;
        }

        
        EstadoBotao estado_confirmar = ui_atualizar_botao(
            &tela->botao_confirmar, false, &entrada);

        if (estado_confirmar == BOTAO_ACIONADO) {
            if (tela->id_selecionado == NENHUM) {
                tela->etapa = JULGAMENTO_RESULTADO;
            } else if (prender_suspeito(tela->jogo,
                                        tela->id_selecionado)) {
                tela->etapa = JULGAMENTO_RESULTADO;
            }
        }

    } else if (tela->etapa == JULGAMENTO_RESULTADO) {
        EstadoBotao estado_continuar = ui_atualizar_botao(
            &tela->botao_continuar, false, &entrada);

        if (estado_continuar == BOTAO_ACIONADO ||
            entrada.confirmar) {
            return JULGAMENTO_CONTINUAR;
        }
    }

    return JULGAMENTO_NENHUMA_ACAO;
}


static void desenhar_retrato_suspeito(const TelaJulgamento *tela,
                                     int id, Rectangle area) {
    if (tela->recursos == NULL) {
        return;
    }

    Texture2D textura = tela->recursos->retratos_suspeitos[id];

    if (textura.id == 0 || textura.width <= 0 ||
        textura.height <= 0) {
        DrawRectangleRec(area, (Color){65, 61, 56, 255});
        return;
    }

    const Suspeito *suspeito = buscar_suspeito(tela->jogo, id);

    Color tonalidade = WHITE;

    if (suspeito != NULL && suspeito->situacao == PRESO) {
        tonalidade = (Color){140, 133, 123, 220};
    } else if (suspeito != NULL && suspeito->situacao == MORTO) {
        tonalidade = (Color){115, 109, 101, 200};
    }

    
    float escala = fminf(
        area.width / (float)textura.width,
        area.height / (float)textura.height
    );

    float largura = (float)textura.width * escala;
    float altura = (float)textura.height * escala;

    Rectangle origem = {
        0.0f, 0.0f,
        (float)textura.width,
        (float)textura.height
    };

    Rectangle destino = {
        area.x + (area.width - largura) / 2.0f,
        area.y + (area.height - altura) / 2.0f,
        largura,
        altura
    };

    DrawTexturePro(
        textura,
        origem,
        destino,
        (Vector2){0.0f, 0.0f},
        0.0f,
        tonalidade
    );

}


void tela_julgamento_desenhar(const TelaJulgamento *tela,
                              const UIFonte *fonte_titulo,
                              const UIFonte *fonte_corpo,
                              const UIFonte *fonte_corpo_bold) {
    if (tela == NULL || tela->jogo == NULL) {
        return;
    }

    DrawRectangle(0, 0, 1280, 720, (Color){30, 28, 26, 255});

    ui_desenhar_texto_centralizado(
        fonte_titulo,
        "JULGAMENTO",
        (Rectangle){0.0f, 45.0f, 1280.0f, 70.0f},
        48.0f,
        RAYWHITE
    );

    if (tela->etapa == JULGAMENTO_ESCOLHENDO) {
        ui_desenhar_texto_centralizado(
            fonte_corpo,
            "Escolha um suspeito para prender ou nao prenda ninguem.",
            (Rectangle){100.0f, 150.0f, 1080.0f, 50.0f},
            24.0f,
            LIGHTGRAY
        );

        
        for (int id = 0; id < TOTAL_SUSPEITOS; id++) {
            Suspeito *suspeito = buscar_suspeito(tela->jogo, id);

            if (suspeito != NULL) {
                Rectangle area_retrato = {
                    tela->botoes_suspeitos[id].area.x + 100.0f,
                    tela->botoes_suspeitos[id].area.y - 115.0f,
                    120.0f,
                    105.0f
                };

                desenhar_retrato_suspeito(tela, id, area_retrato);

                BotaoUI *botao = (BotaoUI *)&tela->botoes_suspeitos[id];

                botao->rotulo = suspeito->nome;

                ui_desenhar_botao(
                    fonte_corpo_bold,
                    botao,
                    false
                );
            }
        }
            ui_desenhar_botao(
                fonte_corpo_bold,
                (BotaoUI *)&tela->botao_nao_prender,
                false
            );

    } else if (tela->etapa == JULGAMENTO_CONFIRMAR) {
        const char *mensagem = "Deseja encerrar o julgamento sem prender ninguem?";

        if (tela->id_selecionado != NENHUM) {
            const Suspeito *suspeito = buscar_suspeito(
                tela->jogo,
                tela->id_selecionado
            );

            if (suspeito != NULL) {
                mensagem = TextFormat(
                    "Deseja prender %s?",
                    suspeito->nome
                );
            }
        }

        ui_desenhar_texto_centralizado(
            fonte_corpo,
            mensagem,
            (Rectangle){140.0f, 260.0f, 1000.0f, 100.0f},
            28.0f,
            RAYWHITE
        );

        ui_desenhar_botao(
            fonte_corpo_bold,
            (BotaoUI *)&tela->botao_cancelar,
            false
        );

       
        ui_desenhar_botao(
            fonte_corpo_bold,
            (BotaoUI *)&tela->botao_confirmar,
            false
        );

    } else if (tela->etapa == JULGAMENTO_RESULTADO) {
        const char *mensagem = "Nenhum suspeito foi preso.";

        if (tela->id_selecionado != NENHUM) {
            const Suspeito *suspeito = buscar_suspeito(
                tela->jogo,
                tela->id_selecionado
            );

            if (suspeito != NULL) {
                mensagem = TextFormat(
                    "%s foi preso.",
                    suspeito->nome
                );
            }
        }

        ui_desenhar_texto_centralizado(
            fonte_corpo,
            mensagem,
            (Rectangle){140.0f, 240.0f, 1000.0f, 80.0f},
            30.0f,
            RAYWHITE
        );

        ui_desenhar_texto_centralizado(
            fonte_corpo,
            TextFormat(
                "Credibilidade: %d",
                tela->jogo->credibilidade
            ),
            (Rectangle){140.0f, 350.0f, 1000.0f, 60.0f},
            26.0f,
            LIGHTGRAY
        );

        int variacao = tela->jogo->credibilidade -
                    tela->credibilidade_anterior;

        ui_desenhar_texto_centralizado(
            fonte_corpo,
            TextFormat("Variacao: %+d pontos", variacao),
            (Rectangle){140.0f, 415.0f, 1000.0f, 60.0f},
            24.0f,
            variacao < 0 ? RED : (variacao > 0 ? GREEN : LIGHTGRAY)
        );


        ui_desenhar_botao(
            fonte_corpo_bold,
            (BotaoUI *)&tela->botao_continuar,
            false
        );
    }
}