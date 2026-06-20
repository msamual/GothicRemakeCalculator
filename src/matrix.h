#ifndef MATRIX_H
#define MATRIX_H

#include "lock.h"

/* Build row `turned`: pin/plate position deltas when tumblers[turned] is pressed [D]. */
bool parse_rule_line(const char *line, int turned, int n, int matrix[][MAX_PLATES]);

bool apply_move_legal(const int state[], const int matrix[][MAX_PLATES],
                      int n, int plate, int dir, int out[]);

#endif
