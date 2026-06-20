#include "../src/lock.h"
#include "../src/matrix.h"
#include "../src/parser.h"
#include "../src/solver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int tests_failed = 0;

#define ASSERT(cond, msg)                          \
    do {                                           \
        tests_run++;                               \
        if (!(cond)) {                             \
            fprintf(stderr, "FAIL: %s\n", msg);    \
            tests_failed++;                        \
        }                                          \
    } while (0)

static void test_apply_move_single_link(void) {
    int matrix[MAX_PLATES][MAX_PLATES] = {{0}};
    matrix[1][0] = LINK_SAME;

    int state[] = {4, 4};
    int out[2];
    apply_move(state, matrix, 2, 0, DIR_RIGHT, out);

    ASSERT(out[0] == 5 && out[1] == 5, "linked plates move together");
}

static void test_pack_unpack(void) {
    int state[] = {5, 3, 6, 7, 2, 7};
    uint32_t key = pack_state(state, 6);
    int out[6];
    unpack_state(key, 6, out);

    ASSERT(memcmp(state, out, sizeof(state)) == 0, "pack/unpack roundtrip");

    int wall_state[] = {1, 1, 1, 1};
    ASSERT(pack_state(wall_state, 4) == 0, "all pins at wall pack to key 0");
    unpack_state(0, 4, out);
    ASSERT(memcmp(wall_state, out, sizeof(wall_state)) == 0,
           "key 0 roundtrip for wall state");
}

static void test_parse_rule_line(void) {
    int matrix[MAX_PLATES][MAX_PLATES] = {{0}};
    ASSERT(parse_rule_line("3r, 6l", 0, 6, matrix), "parse rule line");
    ASSERT(matrix[2][0] == LINK_SAME && matrix[5][0] == LINK_OPP,
           "rule directions stored in matrix column");
    ASSERT(parse_rule_line("-", 1, 6, matrix), "parse empty rule");
}

static void test_parse_fixture(void) {
    LockState lock;
    ParseResult result = parse_lock_path("tests/fixtures/tower_chest.txt", &lock);
    ASSERT(result == PARSE_OK, "parse tower chest fixture");
    ASSERT(lock.n == 6, "fixture has 6 plates");

    int expected[] = {5, 3, 6, 7, 2, 7};
    ASSERT(memcmp(lock.start, expected, sizeof(expected)) == 0, "fixture start positions");

    ASSERT(lock.matrix[2][0] == LINK_SAME, "plate 3 linked with plate 1");
    ASSERT(lock.matrix[5][0] == LINK_OPP, "plate 6 opposes plate 1");
    ASSERT(lock.matrix[5][2] == LINK_SAME, "plate 6 linked with plate 3");
}

static void test_solve_tower_chest(void) {
    LockState lock;
    ASSERT(parse_lock_path("tests/fixtures/tower_chest.txt", &lock) == PARSE_OK,
           "load fixture for solve");

    Solution sol;
    solution_init(&sol);
    SolveResult result = solve_lock(&lock, &sol);
    ASSERT(result == SOLVE_OK, "tower chest solvable");
    ASSERT(sol.count == 52, "tower chest shortest solution length");
    ASSERT(verify_solution(&lock, &sol), "solution replays safely to solved state");

    solution_free(&sol);
}

static void test_solve_from_all_ones(void) {
    LockState lock;
    lock_init(&lock);
    lock.n = 2;
    lock.start[0] = POS_MIN;
    lock.start[1] = POS_MIN;

    Solution sol;
    solution_init(&sol);
    SolveResult result = solve_lock(&lock, &sol);
    ASSERT(result == SOLVE_OK, "independent plates solvable from key 0 state");
    ASSERT(verify_solution(&lock, &sol), "all-ones start solution valid");
    solution_free(&sol);
}

static void test_already_solved(void) {
    LockState lock;
    lock_init(&lock);
    lock.n = 3;
    lock.start[0] = lock.start[1] = lock.start[2] = POS_CENTER;

    Solution sol;
    solution_init(&sol);
    SolveResult result = solve_lock(&lock, &sol);
    ASSERT(result == SOLVE_ALREADY, "centered lock already open");
    ASSERT(sol.count == 0, "no moves for solved lock");
    solution_free(&sol);
}

int main(void) {
    test_apply_move_single_link();
    test_pack_unpack();
    test_parse_rule_line();
    test_parse_fixture();
    test_solve_tower_chest();
    test_solve_from_all_ones();
    test_already_solved();

    if (tests_failed == 0) {
        printf("All %d tests passed.\n", tests_run);
        return 0;
    }

    fprintf(stderr, "%d/%d tests failed.\n", tests_failed, tests_run);
    return 1;
}
