#include "matrix.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const char *skip_ws(const char *s) {
    while (*s && isspace((unsigned char)*s)) {
        s++;
    }
    return s;
}

bool parse_rule_line(const char *line, int turned, int n, int matrix[][MAX_PLATES]) {
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

        int link = (*p == 'r') ? LINK_SAME : LINK_OPP;
        p++;
        matrix[target - 1][turned] = link;

        p = skip_ws(p);
        if (*p == ',') {
            p = skip_ws(p + 1);
        } else if (*p != '\0') {
            return false;
        }
    }

    return true;
}
