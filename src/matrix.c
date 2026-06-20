#include "matrix.h"

#include <ctype.h>
#include <string.h>

static const char *skip_ws(const char *s) {
    while (*s && isspace((unsigned char)*s)) {
        s++;
    }
    return s;
}

bool parse_rule_line(const char *line, int turned, int n, int matrix[][MAX_PLATES]) {
    for (int i = 0; i < n; i++) {
        matrix[turned][i] = 0;
    }
    matrix[turned][turned] = -1;

    const char *p = skip_ws(line);
    if (*p == '-' && (p[1] == '\0' || isspace((unsigned char)p[1]))) {
        return true;
    }

    while (*p) {
        if (!isdigit((unsigned char)*p)) {
            return false;
        }

        int target = 0;
        while (isdigit((unsigned char)*p)) {
            target = target * 10 + (*p - '0');
            p++;
        }

        if (target < 1 || target > n || target == turned + 1) {
            return false;
        }

        p = skip_ws(p);
        if (*p != 'r' && *p != 'l') {
            return false;
        }

        matrix[turned][target - 1] += (*p == 'r') ? -1 : 1;
        p++;

        p = skip_ws(p);
        if (*p == ',') {
            p = skip_ws(p + 1);
        } else if (*p != '\0') {
            return false;
        }
    }

    return true;
}

bool apply_move_legal(const int state[], const int matrix[][MAX_PLATES],
                      int n, int plate, int dir, int out[]) {
    apply_move(state, matrix, n, plate, dir, out);
    return is_valid_state(out, n);
}
