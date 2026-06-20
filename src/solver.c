#include "solver.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define HASH_CAP (1u << 22)
#define INITIAL_NODE_CAP 4096
#define INITIAL_DEQUE_CAP 4096
#define NO_LAST_PLATE (-1)
#define NO_LAST_DIR 0

typedef struct {
    uint32_t pos_key;
    int8_t last_plate;
    int8_t last_dir;
    int16_t segments;
    int16_t moves;
    int parent;
    Move via;
} SearchNode;

typedef struct {
    int16_t segments;
    int16_t moves;
    int node_idx;
} BestEntry;

typedef struct {
    uint64_t *keys;
    BestEntry *best;
    uint32_t cap;
} BestMap;

typedef struct {
    SearchNode *nodes;
    int count;
    int cap;
} NodeStore;

typedef struct {
    int *data;
    int head;
    int tail;
    int cap;
} Deque;

static uint64_t encode_aug(uint32_t pos_key, int last_plate, int last_dir) {
    return (uint64_t)pos_key
         | ((uint64_t)(last_plate + 1) << 24)
         | ((uint64_t)(last_dir + 1) << 28);
}

static uint32_t hash_aug(uint64_t key, uint32_t cap) {
    key ^= key >> 33;
    key *= 0xff51afd7ed558ccdULL;
    key ^= key >> 33;
    key *= 0xc4ceb9fe1a85ec53ULL;
    key ^= key >> 33;
    return (uint32_t)(key & (uint64_t)(cap - 1u));
}

static bool bestmap_init(BestMap *map) {
    map->cap = HASH_CAP;
    map->keys = calloc(map->cap, sizeof(uint64_t));
    map->best = calloc(map->cap, sizeof(BestEntry));
    if (!map->keys || !map->best) {
        free(map->keys);
        free(map->best);
        return false;
    }
    for (uint32_t i = 0; i < map->cap; i++) {
        map->best[i].segments = INT16_MAX;
        map->best[i].node_idx = -1;
    }
    return true;
}

static void bestmap_free(BestMap *map) {
    free(map->keys);
    free(map->best);
    map->keys = NULL;
    map->best = NULL;
    map->cap = 0;
}

static bool is_better(int seg_a, int mov_a, int seg_b, int mov_b) {
    if (seg_a != seg_b) {
        return seg_a < seg_b;
    }
    return mov_a < mov_b;
}

static BestEntry *bestmap_lookup(BestMap *map, uint64_t key) {
    uint32_t idx = hash_aug(key, map->cap);
    for (uint32_t i = 0; i < map->cap; i++) {
        uint32_t slot = (idx + i) & (map->cap - 1u);
        if (map->keys[slot] == 0) {
            map->keys[slot] = key;
            map->best[slot].segments = INT16_MAX;
            map->best[slot].moves = INT16_MAX;
            map->best[slot].node_idx = -1;
            return &map->best[slot];
        }
        if (map->keys[slot] == key) {
            return &map->best[slot];
        }
    }
    return NULL;
}

static bool nodestore_init(NodeStore *store) {
    store->count = 0;
    store->cap = INITIAL_NODE_CAP;
    store->nodes = malloc((size_t)store->cap * sizeof(SearchNode));
    return store->nodes != NULL;
}

static void nodestore_free(NodeStore *store) {
    free(store->nodes);
    store->nodes = NULL;
    store->count = 0;
    store->cap = 0;
}

static bool nodestore_push(NodeStore *store, SearchNode node, int *out_idx) {
    if (store->count >= store->cap) {
        int new_cap = store->cap * 2;
        SearchNode *nodes = realloc(store->nodes, (size_t)new_cap * sizeof(SearchNode));
        if (!nodes) {
            return false;
        }
        store->nodes = nodes;
        store->cap = new_cap;
    }
    *out_idx = store->count;
    store->nodes[store->count++] = node;
    return true;
}

static bool deque_init(Deque *dq) {
    dq->head = 0;
    dq->tail = 0;
    dq->cap = INITIAL_DEQUE_CAP;
    dq->data = malloc((size_t)dq->cap * sizeof(int));
    return dq->data != NULL;
}

static void deque_free(Deque *dq) {
    free(dq->data);
    dq->data = NULL;
    dq->cap = 0;
}

static bool deque_empty(const Deque *dq) {
    return dq->head == dq->tail;
}

static bool deque_push_front(Deque *dq, int value) {
    if (dq->head == 0) {
        int new_cap = dq->cap * 2;
        int *data = malloc((size_t)new_cap * sizeof(int));
        if (!data) {
            return false;
        }
        int len = dq->tail - dq->head;
        memcpy(data + new_cap - len, dq->data + dq->head, (size_t)len * sizeof(int));
        free(dq->data);
        dq->data = data;
        dq->head = new_cap - len;
        dq->tail = new_cap;
        dq->cap = new_cap;
    }
    dq->data[--dq->head] = value;
    return true;
}

static bool deque_push_back(Deque *dq, int value) {
    if (dq->tail >= dq->cap) {
        int new_cap = dq->cap * 2;
        int *data = realloc(dq->data, (size_t)new_cap * sizeof(int));
        if (!data) {
            return false;
        }
        dq->data = data;
        dq->cap = new_cap;
    }
    dq->data[dq->tail++] = value;
    return true;
}

static bool deque_pop_front(Deque *dq, int *value) {
    if (deque_empty(dq)) {
        return false;
    }
    *value = dq->data[dq->head++];
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

int solution_line_count(const Solution *sol) {
    int lines = 0;
    int i = 0;
    while (i < sol->count) {
        lines++;
        Move m = sol->moves[i];
        i++;
        while (i < sol->count &&
               sol->moves[i].plate == m.plate &&
               sol->moves[i].dir == m.dir) {
            i++;
        }
    }
    return lines;
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
    for (int idx = end_idx; store->nodes[idx].parent >= 0; idx = store->nodes[idx].parent) {
        length++;
    }

    if (!solution_reserve(sol, length)) {
        return false;
    }

    sol->count = length;
    int write = length - 1;
    for (int idx = end_idx; store->nodes[idx].parent >= 0; idx = store->nodes[idx].parent) {
        sol->moves[write--] = store->nodes[idx].via;
    }
    return true;
}

static int next_segments(const SearchNode *node, int plate, int dir) {
    if (node->last_plate < 0) {
        return 1;
    }
    if (plate == node->last_plate && dir == node->last_dir) {
        return node->segments;
    }
    return node->segments + 1;
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

    BestMap bestmap;
    NodeStore nodes;
    Deque dq;

    if (!bestmap_init(&bestmap) || !nodestore_init(&nodes) || !deque_init(&dq)) {
        bestmap_free(&bestmap);
        nodestore_free(&nodes);
        deque_free(&dq);
        return SOLVE_ERR;
    }

    uint32_t start_key = pack_state(lock->start, lock->n);
    SearchNode start = {
        .pos_key = start_key,
        .last_plate = NO_LAST_PLATE,
        .last_dir = NO_LAST_DIR,
        .segments = 0,
        .moves = 0,
        .parent = -1,
        .via = {0, 0},
    };

    int start_idx = 0;
    if (!nodestore_push(&nodes, start, &start_idx) ||
        !deque_push_back(&dq, start_idx)) {
        bestmap_free(&bestmap);
        nodestore_free(&nodes);
        deque_free(&dq);
        return SOLVE_ERR;
    }

    BestEntry *start_best = bestmap_lookup(&bestmap, encode_aug(start_key, NO_LAST_PLATE, NO_LAST_DIR));
    if (!start_best) {
        bestmap_free(&bestmap);
        nodestore_free(&nodes);
        deque_free(&dq);
        return SOLVE_ERR;
    }
    start_best->segments = 0;
    start_best->moves = 0;
    start_best->node_idx = start_idx;

    int state[MAX_PLATES];
    int next[MAX_PLATES];
    int answer_idx = -1;
    int answer_seg = INT16_MAX;
    int answer_mov = INT16_MAX;
    SolveResult result = SOLVE_NO_PATH;

    while (!deque_empty(&dq)) {
        int cur_idx = 0;
        if (!deque_pop_front(&dq, &cur_idx)) {
            break;
        }

        SearchNode cur = nodes.nodes[cur_idx];
        uint64_t cur_aug = encode_aug(cur.pos_key, cur.last_plate, cur.last_dir);
        BestEntry *cur_best = bestmap_lookup(&bestmap, cur_aug);
        if (!cur_best || cur_best->node_idx != cur_idx) {
            continue;
        }

        unpack_state(cur.pos_key, lock->n, state);

        for (int plate = 0; plate < lock->n; plate++) {
            for (int dir = -1; dir <= 1; dir += 2) {
                apply_move(state, lock->matrix, lock->n, plate, dir, next);
                if (!is_valid_state(next, lock->n)) {
                    continue;
                }

                int new_seg = next_segments(&cur, plate, dir);
                int new_mov = cur.moves + 1;
                uint32_t next_key = pack_state(next, lock->n);
                uint64_t next_aug = encode_aug(next_key, plate, dir);

                BestEntry *entry = bestmap_lookup(&bestmap, next_aug);
                if (!entry) {
                    result = SOLVE_ERR;
                    goto cleanup;
                }

                if (!is_better(new_seg, new_mov, entry->segments, entry->moves)) {
                    continue;
                }

                SearchNode node = {
                    .pos_key = next_key,
                    .last_plate = (int8_t)plate,
                    .last_dir = (int8_t)dir,
                    .segments = (int16_t)new_seg,
                    .moves = (int16_t)new_mov,
                    .parent = cur_idx,
                    .via = {plate, dir},
                };

                int node_idx = 0;
                if (!nodestore_push(&nodes, node, &node_idx)) {
                    result = SOLVE_ERR;
                    goto cleanup;
                }

                entry->segments = (int16_t)new_seg;
                entry->moves = (int16_t)new_mov;
                entry->node_idx = node_idx;

                bool same_segment = (new_seg == cur.segments);
                if (!same_segment ? !deque_push_back(&dq, node_idx)
                                  : !deque_push_front(&dq, node_idx)) {
                    result = SOLVE_ERR;
                    goto cleanup;
                }

                if (is_solved(next, lock->n) &&
                    is_better(new_seg, new_mov, answer_seg, answer_mov)) {
                    answer_seg = new_seg;
                    answer_mov = new_mov;
                    answer_idx = node_idx;
                }
            }
        }
    }

    if (answer_idx >= 0) {
        if (reconstruct_path(&nodes, answer_idx, sol)) {
            result = SOLVE_OK;
        } else {
            result = SOLVE_ERR;
        }
    }

cleanup:
    bestmap_free(&bestmap);
    nodestore_free(&nodes);
    deque_free(&dq);
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
