#include "lock.h"
#include "output.h"
#include "parser.h"
#include "solver.h"

#include <stdio.h>
#include <string.h>

static int run_solver(FILE *in) {
    LockState lock;
    ParseResult parse_result = parse_lock_file(in, &lock);
    if (parse_result != PARSE_OK) {
        fprintf(stderr, "Error: %s\n", parse_result_message(parse_result));
        return 1;
    }

    Solution sol;
    solution_init(&sol);
    SolveResult solve_result = solve_lock(&lock, &sol);

    if (solve_result == SOLVE_ALREADY) {
        printf("%s\nAlready open.\n", lock.name);
        solution_free(&sol);
        return 0;
    }

    if (solve_result != SOLVE_OK) {
        fprintf(stderr, "Error: %s\n", solve_result_message(solve_result));
        solution_free(&sol);
        return 1;
    }

    print_solution(stdout, &lock, &sol);
    solution_free(&sol);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        print_help(stdout);
        return 0;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_help(stdout);
        return 0;
    }

    if (strcmp(argv[1], "--template") == 0) {
        print_template(stdout);
        return 0;
    }

    if (strcmp(argv[1], "-") == 0) {
        return run_solver(stdin);
    }

    FILE *in = fopen(argv[1], "r");
    if (!in) {
        fprintf(stderr, "Error: cannot read input file\n");
        return 1;
    }

    int code = run_solver(in);
    fclose(in);
    return code;
}
