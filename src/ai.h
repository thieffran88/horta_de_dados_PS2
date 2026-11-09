#ifndef HORTA_AI_H
#define HORTA_AI_H
#include "game.h"

/* Escolhe a jogada do jogador atual (g->current). Sempre encontra uma
 * jogada valida enquanto houver mais de um dado no pool. */
void ai_choose(const Game *g, Move *out);

#endif
