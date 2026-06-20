#include "solver.h"

#include <stdlib.h>
#include <string.h>

#define HASH_CAP (1u << 22)
#define INITIAL_NODE_CAP 4096
#define INITIAL_QUEUE_CAP 4096

typedef struct {
    uint32_t *keys;
    int *parent;
    Move *via;
    int count;
    int cap;
} NodeStore;

typedef struct {
    int *data;
    int head;
    int tail;
    int cap;
} IntQueue;

typedef struct {
    uint32_t *slots;
    uint32_t cap;
} VisitedSet;

static uint32_t hash_key(uint32_t key, uint32_t cap) {
    key ^= key >> 16;
    key *= 0x7feb352du;
    key ^= key >> 15;
    key *= 0x846ca68bu;
    key ^= key >> 16;
    return key & (cap - 1u);
}

/* Slots use 0 for empty; packed state 0 is valid, so store key + 1. */
static uint32_t visited_encode(uint32_t key) {
    return key + 1u;
}

static bool visited_init(VisitedSet *set) {
    set->cap = HASH_CAP;
    set->slots = calloc(set->cap, sizeof(uint32_t));
    return set->slots != NULL;
}

static void visited_free(VisitedSet *set) {
    free(set->slots);
    set->slots = NULL;
    set->cap = 0;
}

static bool visited_contains(const VisitedSet *set, uint32_t key) {
    uint32_t encoded = visited_encode(key);
    uint32_t idx = hash_key(key, set->cap);
    for (uint32_t i = 0; i < set->cap; i++) {
        uint32_t slot = (idx + i) & (set->cap - 1u);
        uint32_t entry = set->slots[slot];
        if (entry == 0) {
            return false;
        }
        if (entry == encoded) {
            return true;
        }
    }
    return false;
}

static bool visited_insert(VisitedSet *set, uint32_t key) {
    uint32_t encoded = visited_encode(key);
    uint32_t idx = hash_key(key, set->cap);
    for (uint32_t i = 0; i < set->cap; i++) {
        uint32_t slot = (idx + i) & (set->cap - 1u);
        uint32_t entry = set->slots[slot];
        if (entry == 0) {
            set->slots[slot] = encoded;
            return true;
        }
        if (entry == encoded) {
            return false;
        }
    }
    return false;
}

static bool node_store_init(NodeStore *store) {
    store->count = 0;
    store->cap = INITIAL_NODE_CAP;
    store->keys = malloc((size_t)store->cap * sizeof(uint32_t));
    store->parent = malloc((size_t)store->cap * sizeof(int));
    store->via = malloc((size_t)store->cap * sizeof(Move));
    return store->keys && store->parent && store->via;
}

static void node_store_free(NodeStore *store) {
    free(store->keys);
    free(store->parent);
    free(store->via);
    store->keys = NULL;
    store->parent = NULL;
    store->via = NULL;
    store->count = 0;
    store->cap = 0;
}

static bool node_store_push(NodeStore *store, uint32_t key, int parent, Move via, int *out_idx) {
    if (store->count >= store->cap) {
        int new_cap = store->cap * 2;
        uint32_t *keys = realloc(store->keys, (size_t)new_cap * sizeof(uint32_t));
        int *parent_arr = realloc(store->parent, (size_t)new_cap * sizeof(int));
        Move *via_arr = realloc(store->via, (size_t)new_cap * sizeof(Move));
        if (!keys || !parent_arr || !via_arr) {
            free(keys);
            free(parent_arr);
            free(via_arr);
            return false;
        }
        store->keys = keys;
        store->parent = parent_arr;
        store->via = via_arr;
        store->cap = new_cap;
    }

    *out_idx = store->count;
    store->keys[store->count] = key;
    store->parent[store->count] = parent;
    store->via[store->count] = via;
    store->count++;
    return true;
}

static bool queue_init(IntQueue *q) {
    q->head = 0;
    q->tail = 0;
    q->cap = INITIAL_QUEUE_CAP;
    q->data = malloc((size_t)q->cap * sizeof(int));
    return q->data != NULL;
}

static void queue_free(IntQueue *q) {
    free(q->data);
    q->data = NULL;
    q->cap = 0;
}

static bool queue_empty(const IntQueue *q) {
    return q->head == q->tail;
}

static bool queue_push(IntQueue *q, int value) {
    if (q->tail >= q->cap) {
        int new_cap = q->cap * 2;
        int *data = realloc(q->data, (size_t)new_cap * sizeof(int));
        if (!data) {
            return false;
        }
        q->data = data;
        q->cap = new_cap;
    }
    q->data[q->tail++] = value;
    return true;
}

static bool queue_pop(IntQueue *q, int *value) {
    if (queue_empty(q)) {
        return false;
    }
    *value = q->data[q->head++];
    return true;
}

void solution_init(Solution *sol) {
    sol->moves = NULL;
    sol->count = 0;
    sol->cap = 0;
}

void solution_free(Solution *sol) {
    free(sol->moves);
    sol->moves = NULL;
    sol->count = 0;
    sol->cap = 0;
}

static bool solution_reserve(Solution *sol, int cap) {
    if (cap <= sol->cap) {
        return true;
    }
    Move *moves = realloc(sol->moves, (size_t)cap * sizeof(Move));
    if (!moves) {
        return false;
    }
    sol->moves = moves;
    sol->cap = cap;
    return true;
}

static bool reconstruct_path(const NodeStore *store, int end_idx, Solution *sol) {
    int length = 0;
    for (int idx = end_idx; store->parent[idx] >= 0; idx = store->parent[idx]) {
        length++;
    }

    if (!solution_reserve(sol, length)) {
        return false;
    }

    sol->count = length;
    int write = length - 1;
    for (int idx = end_idx; store->parent[idx] >= 0; idx = store->parent[idx]) {
        sol->moves[write--] = store->via[idx];
    }
    return true;
}

SolveResult solve_lock(const LockState *lock, Solution *sol) {
    solution_free(sol);
    solution_init(sol);

    if (!lock || lock->n < MIN_PLATES || lock->n > MAX_PLATES) {
        return SOLVE_ERR;
    }
    if (!is_valid_state(lock->start, lock->n)) {
        return SOLVE_ERR;
    }
    if (is_solved(lock->start, lock->n)) {
        return SOLVE_ALREADY;
    }

    VisitedSet visited;
    NodeStore nodes;
    IntQueue queue;

    if (!visited_init(&visited) || !node_store_init(&nodes) || !queue_init(&queue)) {
        visited_free(&visited);
        node_store_free(&nodes);
        queue_free(&queue);
        return SOLVE_ERR;
    }

    uint32_t start_key = pack_state(lock->start, lock->n);
    int start_idx = 0;
    if (!node_store_push(&nodes, start_key, -1, (Move){0, 0}, &start_idx) ||
        !visited_insert(&visited, start_key) ||
        !queue_push(&queue, start_idx)) {
        visited_free(&visited);
        node_store_free(&nodes);
        queue_free(&queue);
        return SOLVE_ERR;
    }

    int state[MAX_PLATES];
    int next[MAX_PLATES];
    SolveResult result = SOLVE_NO_PATH;

    while (!queue_empty(&queue)) {
        int cur_idx = 0;
        if (!queue_pop(&queue, &cur_idx)) {
            break;
        }

        unpack_state(nodes.keys[cur_idx], lock->n, state);

        for (int plate = 0; plate < lock->n; plate++) {
            for (int dir = -1; dir <= 1; dir += 2) {
                apply_move(state, lock->matrix, lock->n, plate, dir, next);
                if (!is_valid_state(next, lock->n)) {
                    continue;
                }

                uint32_t next_key = pack_state(next, lock->n);
                if (visited_contains(&visited, next_key)) {
                    continue;
                }

                Move move = {plate, dir};
                int next_idx = 0;
                if (!node_store_push(&nodes, next_key, cur_idx, move, &next_idx) ||
                    !visited_insert(&visited, next_key) ||
                    !queue_push(&queue, next_idx)) {
                    result = SOLVE_ERR;
                    goto cleanup;
                }

                if (is_solved(next, lock->n)) {
                    if (!reconstruct_path(&nodes, next_idx, sol)) {
                        result = SOLVE_ERR;
                    } else {
                        result = SOLVE_OK;
                    }
                    goto cleanup;
                }
            }
        }
    }

cleanup:
    visited_free(&visited);
    node_store_free(&nodes);
    queue_free(&queue);
    return result;
}

bool verify_solution(const LockState *lock, const Solution *sol) {
    int state[MAX_PLATES];
    int next[MAX_PLATES];
    memcpy(state, lock->start, (size_t)lock->n * sizeof(int));

    for (int i = 0; i < sol->count; i++) {
        Move m = sol->moves[i];
        if (m.plate < 0 || m.plate >= lock->n) {
            return false;
        }
        if (m.dir != DIR_LEFT && m.dir != DIR_RIGHT) {
            return false;
        }
        apply_move(state, lock->matrix, lock->n, m.plate, m.dir, next);
        if (!is_valid_state(next, lock->n)) {
            return false;
        }
        memcpy(state, next, (size_t)lock->n * sizeof(int));
    }

    return is_solved(state, lock->n);
}
