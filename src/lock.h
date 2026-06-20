#ifndef LOCK_H
#define LOCK_H

#include <stdbool.h>
#include <stdint.h>

#define POS_MIN 1
#define POS_MAX 7
#define POS_CENTER 4
#define MIN_PLATES 2
#define MAX_PLATES 8

#define LINK_NONE 0
#define LINK_SAME 1
#define LINK_OPP -1

#define DIR_LEFT -1
#define DIR_RIGHT 1

typedef struct {
    int plate; /* 0-based index */
    int dir;   /* +1 = D (right), -1 = A (left) */
} Move;

typedef struct {
    char name[256];
    int n;
    int matrix[MAX_PLATES][MAX_PLATES];
    int start[MAX_PLATES];
} LockState;

void lock_init(LockState *lock);
void apply_move(const int state[], const int matrix[][MAX_PLATES],
                int n, int plate, int dir, int out[]);
bool is_valid_state(const int state[], int n);
bool is_solved(const int state[], int n);
uint32_t pack_state(const int state[], int n);
void unpack_state(uint32_t key, int n, int out[]);

#endif
