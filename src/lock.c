#include "lock.h"

#include <stdint.h>
#include <string.h>

void lock_init(LockState *lock) {
    memset(lock, 0, sizeof(*lock));
    lock->name[0] = '\0';
}

void apply_move(const int state[], const int matrix[][MAX_PLATES],
                int n, int plate, int dir, int out[]) {
    /* Plate positions 1–7 (1 = right wall, 7 = left). matrix[t][i] is the delta
     * for plate i when [D] is pressed on t; [A] subtracts the same row. */
    int sgn = (dir == DIR_RIGHT) ? 1 : -1;
    for (int i = 0; i < n; i++) {
        out[i] = state[i] + sgn * matrix[plate][i];
    }
}

bool is_valid_state(const int state[], int n) {
    for (int i = 0; i < n; i++) {
        if (state[i] < POS_MIN || state[i] > POS_MAX) {
            return false;
        }
    }
    return true;
}

bool is_solved(const int state[], int n) {
    for (int i = 0; i < n; i++) {
        if (state[i] != POS_CENTER) {
            return false;
        }
    }
    return true;
}

uint32_t pack_state(const int state[], int n) {
    uint32_t key = 0;
    for (int i = 0; i < n; i++) {
        key |= (uint32_t)(state[i] - POS_MIN) << (i * 3);
    }
    return key;
}

void unpack_state(uint32_t key, int n, int out[]) {
    for (int i = 0; i < n; i++) {
        out[i] = (int)((key >> (i * 3)) & 7u) + POS_MIN;
    }
}
