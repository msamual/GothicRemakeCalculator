#ifndef MATRIX_H
#define MATRIX_H

#include "lock.h"

/* Parse a rules line like "3r, 6l" or "-" into matrix row `turned` (0-based). */
bool parse_rule_line(const char *line, int turned, int n, int matrix[][MAX_PLATES]);

#endif
