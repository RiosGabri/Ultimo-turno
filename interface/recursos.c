#include "recursos.h"

#include <math.h>
#include <stdio.h>

#include "paleta.h"

static Texture2D carregar_textura(const char *relativo) {
    Texture2D textura = {0};
    char caminho[1024];

    if (recursos_caminho(relativo, caminho, sizeof caminho) == NULL) {
        TraceLog(LOG_WARNING, "Recursos: '%s' nao encontrado.", relativo);
        return textura;
    }

    textura = LoadTexture(caminho);
    if (textura.id == 0) {
        TraceLog(LOG_WARNING, "Recursos: falha ao carregar '%s'.", caminho);
        return (Texture2D){0};
    }

    SetTextureFilter(textura, TEXTURE_FILTER_BILINEAR);
    TraceLog(LOG_INFO, "Recursos: '%s' carregado (%dx%d).", caminho,
             textura.width, textura.height);
    return textura;
}

const char *recursos_caminho(const char *relativo, char *destino,
                             size_t tamanho) {
    if (relativo == NULL || destino == NULL || tamanho == 0) {
        return NULL;
    }

    if (FileExists(relativo)) {
        snprintf(destino, tamanho, "%s", relativo);
        return destino;
    }

    const char *pasta_aplicacao = GetApplicationDirectory();
    if (pasta_aplicacao != NULL) {
        snprintf(destino, tamanho, "%s%s", pasta_aplicacao, relativo);
        if (FileExists(destino)) {
            return destino;
        }
    }

    snprintf(destino, tamanho, "../%s", relativo);
    if (FileExists(destino)) {
        return destino;
    }

    snprintf(destino, tamanho, "../../%s", relativo);
    if (FileExists(destino)) {
        return destino;
    }

    return NULL;
}

void recursos_carregar(Recursos *recursos) {
    if (recursos == NULL) {
        return;
    }

    recursos->fabrica = carregar_textura("assets/image/fabrica.png");
    recursos->marple = carregar_textura("assets/image/marple.png");
    recursos->hercule = carregar_textura("assets/image/hercule.png");
}

void recursos_descarregar(Recursos *recursos) {
    if (recursos == NULL) {
        return;
    }

    if (recursos->fabrica.id != 0) {
        UnloadTexture(recursos->fabrica);
        recursos->fabrica = (Texture2D){0};
    }
    if (recursos->marple.id != 0) {
        UnloadTexture(recursos->marple);
        recursos->marple = (Texture2D){0};
    }
    if (recursos->hercule.id != 0) {
        UnloadTexture(recursos->hercule);
        recursos->hercule = (Texture2D){0};
    }
}

void recursos_desenhar_textura_cover(Texture2D textura, Rectangle destino) {
    if (textura.id == 0 || destino.width <= 0.0f || destino.height <= 0.0f) {
        return;
    }

    float escala_x = destino.width / (float)textura.width;
    float escala_y = destino.height / (float)textura.height;
    float escala = fmaxf(escala_x, escala_y);

    Rectangle origem = {
        (textura.width - destino.width / escala) * 0.5f,
        (textura.height - destino.height / escala) * 0.5f,
        destino.width / escala,
        destino.height / escala,
    };

    DrawTexturePro(textura, origem, destino, (Vector2){0.0f, 0.0f}, 0.0f,
                   WHITE);
}

void recursos_desenhar_fundo(const Recursos *recursos) {
    if (recursos == NULL || recursos->fabrica.id == 0) {
        ClearBackground(COR_CARVAO);
        return;
    }

    Rectangle tela = {0.0f, 0.0f, (float)GetScreenWidth(),
                      (float)GetScreenHeight()};
    recursos_desenhar_textura_cover(recursos->fabrica, tela);
}
