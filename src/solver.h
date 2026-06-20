#ifndef SOLVER_H
#define SOLVER_H

#include "lock.h"

typedef enum {
    SOLVE_OK = 0,
    SOLVE_ALREADY,
    SOLVE_NO_PATH,
    SOLVE_ERR
} SolveResult;

typedef struct {
    Move *moves;
    int count;
    int cap;
} Solution;

void solution_init(Solution *sol);
void solution_free(Solution *sol);

SolveResult solve_lock(const LockState *lock, Solution *sol);

/* Replay solution; returns false if any move goes out of bounds. */
bool verify_solution(const LockState *lock, const Solution *sol);

#endif
