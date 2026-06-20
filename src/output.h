#ifndef OUTPUT_H
#define OUTPUT_H

#include "lock.h"
#include "parser.h"
#include "solver.h"
#include <stdio.h>

void print_template(FILE *out);
void print_help(FILE *out);
const char *parse_result_message(ParseResult result);
const char *solve_result_message(SolveResult result);
void print_solution(FILE *out, const LockState *lock, const Solution *sol);

#endif
