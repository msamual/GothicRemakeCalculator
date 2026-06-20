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

static void test_apply_move_row_delta(void) {
    LockState lock;
    lock_init(&lock);
    lock.n = 2;
    ASSERT(parse_rule_line("2r", 0, 2, lock.matrix), "parse row delta rule");

    int state[] = {4, 4};
    int out[2];
    apply_move(state, lock.matrix, 2, 0, DIR_RIGHT, out);
    ASSERT(out[0] == 3 && out[1] == 3, "D applies row delta to all plates");

    apply_move(out, lock.matrix, 2, 0, DIR_LEFT, out);
    ASSERT(out[0] == 4 && out[1] == 4, "A subtracts the same row");
}

static void test_pack_unpack(void) {
    int state[] = {5, 3, 6, 7, 2, 7};
    uint32_t key = pack_state(state, 6);
    int out[6];
    unpack_state(key, 6, out);

    ASSERT(memcmp(state, out, sizeof(state)) == 0, "pack/unpack roundtrip");
}

static void test_parse_rule_line(void) {
    int matrix[MAX_PLATES][MAX_PLATES] = {{0}};
    ASSERT(parse_rule_line("3r, 6l", 0, 6, matrix), "parse rule line");
    ASSERT(matrix[0][0] == -1 && matrix[0][2] == -1 && matrix[0][5] == 1,
           "row stores D-press deltas");
    ASSERT(parse_rule_line("-", 1, 6, matrix), "parse empty rule");
    ASSERT(matrix[1][1] == -1, "empty rule still has self delta -1");
}

static void test_parse_fixture(void) {
    LockState lock;
    ParseResult result = parse_lock_path("tests/fixtures/tower_chest.txt", &lock);
    ASSERT(result == PARSE_OK, "parse tower chest fixture");
    ASSERT(lock.n == 6, "fixture has 6 plates");

    int expected[] = {5, 3, 6, 7, 2, 7};
    ASSERT(memcmp(lock.start, expected, sizeof(expected)) == 0, "fixture start positions");
    ASSERT(lock.matrix[0][2] == -1 && lock.matrix[0][5] == 1, "tower rule 1 row");
}

static void test_solve_tower_chest(void) {
    LockState lock;
    ASSERT(parse_lock_path("tests/fixtures/tower_chest.txt", &lock) == PARSE_OK,
           "load fixture for solve");

    Solution sol;
    solution_init(&sol);
    SolveResult result = solve_lock(&lock, &sol);
    ASSERT(result == SOLVE_OK, "tower chest solvable");
    ASSERT(sol.count == 52, "tower chest optimal move count");
    ASSERT(verify_solution(&lock, &sol), "solution replays safely to solved state");

    solution_free(&sol);
}

static void test_input_fixture(void) {
    LockState lock;
    ASSERT(parse_lock_path("tests/fixtures/input.txt", &lock) == PARSE_OK,
           "parse input fixture");

    Solution sol;
    solution_init(&sol);
    ASSERT(solve_lock(&lock, &sol) == SOLVE_OK, "input fixture solvable");
    ASSERT(verify_solution(&lock, &sol), "input fixture solution valid");
    solution_free(&sol);
}

static void test_minimize_lines_not_moves(void) {
    LockState lock;
    lock_init(&lock);
    lock.n = 2;
    lock.start[0] = 7;
    lock.start[1] = 7;
    ASSERT(parse_rule_line("-", 0, 2, lock.matrix), "init row 0");
    ASSERT(parse_rule_line("-", 1, 2, lock.matrix), "init row 1");

    Solution sol;
    solution_init(&sol);
    SolveResult result = solve_lock(&lock, &sol);
    ASSERT(result == SOLVE_OK, "two-plate lock solvable");
    ASSERT(solution_line_count(&sol) == 2, "prefers 2 lines over 6 alternating moves");
    ASSERT(sol.count == 6, "still uses minimum total moves here");
    ASSERT(verify_solution(&lock, &sol), "line-optimal solution valid");

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
    test_apply_move_row_delta();
    test_pack_unpack();
    test_parse_rule_line();
    test_parse_fixture();
    test_solve_tower_chest();
    test_input_fixture();
    test_minimize_lines_not_moves();
    test_already_solved();

    if (tests_failed == 0) {
        printf("All %d tests passed.\n", tests_run);
        return 0;
    }

    fprintf(stderr, "%d/%d tests failed.\n", tests_failed, tests_run);
    return 1;
}
