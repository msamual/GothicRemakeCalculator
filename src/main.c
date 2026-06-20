#include "lock.h"
#include "output.h"
#include "parser.h"
#include "solver.h"

#include <stdio.h>
#include <string.h>

static int run_solver(FILE *in, bool json_mode) {
    LockState lock;
    ParseResult parse_result = parse_lock_file(in, &lock);
    if (parse_result != PARSE_OK) {
        if (json_mode) {
            print_json_error(stdout, parse_result_message(parse_result));
        } else {
            fprintf(stderr, "Error: %s\n", parse_result_message(parse_result));
        }
        return 1;
    }

    Solution sol;
    solution_init(&sol);
    SolveResult solve_result = solve_lock(&lock, &sol);

    if (solve_result == SOLVE_ALREADY) {
        if (json_mode) {
            print_json_already_open(stdout, &lock);
        } else {
            printf("%s\nAlready open.\n", lock.name);
        }
        solution_free(&sol);
        return 0;
    }

    if (solve_result != SOLVE_OK) {
        if (json_mode) {
            print_json_error(stdout, solve_result_message(solve_result));
        } else {
            fprintf(stderr, "Error: %s\n", solve_result_message(solve_result));
        }
        solution_free(&sol);
        return 1;
    }

    if (json_mode) {
        print_json_solution(stdout, &lock, &sol);
    } else {
        print_solution(stdout, &lock, &sol);
    }
    solution_free(&sol);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        print_help(stdout);
        return 0;
    }

    bool json_mode = false;
    const char *input = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_help(stdout);
            return 0;
        }
        if (strcmp(argv[i], "--template") == 0) {
            print_template(stdout);
            return 0;
        }
        if (strcmp(argv[i], "--json") == 0) {
            json_mode = true;
            continue;
        }
        if (input != NULL) {
            fprintf(stderr, "Error: unexpected argument: %s\n", argv[i]);
            return 1;
        }
        input = argv[i];
    }

    if (input == NULL) {
        print_help(stdout);
        return 0;
    }

    if (strcmp(input, "-") == 0) {
        return run_solver(stdin, json_mode);
    }

    FILE *in = fopen(input, "r");
    if (!in) {
        if (json_mode) {
            print_json_error(stdout, "cannot read input file");
        } else {
            fprintf(stderr, "Error: cannot read input file\n");
        }
        return 1;
    }

    int code = run_solver(in, json_mode);
    fclose(in);
    return code;
}
