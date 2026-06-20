#include "output.h"

#include "parser.h"

void print_template(FILE *out) {
    fputs(
        "Name: my lock\n"
        "Rules:\n"
        "1: 3r, 6l\n"
        "2: -\n"
        "3: 1r, 4l, 6r\n"
        "4: 2r, 5r, 6l\n"
        "5: -\n"
        "6: 3l\n"
        "Start:\n"
        "[5, 3, 6, 7, 2, 7]\n",
        out);
}

void print_help(FILE *out) {
    fputs(
        "Gothic Lock — lockpicking solver for Gothic 1 Remake\n"
        "\n"
        "Usage:\n"
        "  gothic-lock [file]      Solve lock from input file\n"
        "  gothic-lock -           Read lock definition from stdin\n"
        "  gothic-lock --template  Print example input format\n"
        "  gothic-lock --help      Show this help\n"
        "\n"
        "Input format:\n"
        "  Name: optional lock name\n"
        "  Rules:\n"
        "    1: 3r, 6l     (plate 3 moves right, plate 6 moves left when plate 1 turns right)\n"
        "    2: -          (no couplings)\n"
        "  Start:\n"
        "    [5, 3, 6, 7, 2, 7]   (plate positions: 1 = right wall, 7 = left, goal 4)\n"
        "\n"
        "Rules are recorded by pressing [D] on each tumbler. [A] subtracts the same row.\n"
        "\n"
        "In game:\n"
        "  D — slide plate right, A — slide plate left\n"
        "  W/S — switch between tumblers\n",
        out);
}

const char *parse_result_message(ParseResult result) {
    switch (result) {
    case PARSE_OK:
        return "ok";
    case PARSE_ERR_IO:
        return "cannot read input file";
    case PARSE_ERR_FORMAT:
        return "invalid input format";
    case PARSE_ERR_PLATES:
        return "plate count mismatch between rules and start positions";
    case PARSE_ERR_POSITIONS:
        return "invalid start positions (must be 1-7)";
    case PARSE_ERR_RULES:
        return "invalid rules line";
    default:
        return "parse error";
    }
}

const char *solve_result_message(SolveResult result) {
    switch (result) {
    case SOLVE_OK:
        return "ok";
    case SOLVE_ALREADY:
        return "lock is already open";
    case SOLVE_NO_PATH:
        return "no safe path found";
    case SOLVE_ERR:
        return "solver error";
    default:
        return "unknown error";
    }
}

static const char *times_word(int n) {
    int mod10 = n % 10;
    int mod100 = n % 100;
    if (mod10 == 1 && mod100 != 11) {
        return "раз";
    }
    if (mod10 >= 2 && mod10 <= 4 && (mod100 < 12 || mod100 > 14)) {
        return "раза";
    }
    return "раз";
}

void print_solution(FILE *out, const LockState *lock, const Solution *sol) {
    if (lock->name[0] != '\0') {
        fprintf(out, "%s\n", lock->name);
    }
    int lines = solution_line_count(sol);
    fprintf(out, "Решение (%d строк, %d шагов):\n", lines, sol->count);

    int i = 0;
    while (i < sol->count) {
        Move m = sol->moves[i];
        int count = 1;
        while (i + count < sol->count &&
               sol->moves[i + count].plate == m.plate &&
               sol->moves[i + count].dir == m.dir) {
            count++;
        }

        const char *dir = (m.dir == DIR_RIGHT) ? "вправо" : "влево";
        fprintf(out, "  %d пластина %d %s %s\n",
                m.plate + 1, count, times_word(count), dir);
        i += count;
    }
}
