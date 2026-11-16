#ifndef TEXTO_H
#define TEXTO_H

/*
 * Copia uma string para memória alocada com malloc.
 * Substitui strdup(), que não faz parte do C padrão e não está disponível
 * em todos os compiladores/plataformas (por exemplo, MSVC no Windows).
 *
 * Retorna NULL se a origem for NULL ou se a alocação falhar.
 * O chamador é responsável por liberar o resultado com free().
 */
char *copiar_texto(const char *origem);

#endif
