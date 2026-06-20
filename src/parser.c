#include "parser.h"

#include "matrix.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static char *trim(char *s) {
    while (*s && isspace((unsigned char)*s)) {
        s++;
    }
    if (*s == '\0') {
        return s;
    }
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) {
        *end-- = '\0';
    }
    return s;
}

static bool parse_start_array(const char *line, int *out, int *n) {
    const char *p = strchr(line, '[');
    if (!p) {
        return false;
    }
    p++;

    int count = 0;
    while (*p && *p != ']') {
        while (*p && (isspace((unsigned char)*p) || *p == ',')) {
            p++;
        }
        if (*p == ']') {
            break;
        }
        if (!isdigit((unsigned char)*p)) {
            return false;
        }

        char *end = NULL;
        long val = strtol(p, &end, 10);
        if (end == p || val < POS_MIN || val > POS_MAX) {
            return false;
        }

        if (count >= MAX_PLATES) {
            return false;
        }
        out[count++] = (int)val;
        p = end;
    }

    if (count < MIN_PLATES || count > MAX_PLATES) {
        return false;
    }

    *n = count;
    return true;
}

static bool parse_rule_header(const char *line, int *plate_num, char **rest) {
    const char *p = line;
    while (isspace((unsigned char)*p)) {
        p++;
    }

    if (!isdigit((unsigned char)*p)) {
        return false;
    }

    char *end = NULL;
    long num = strtol(p, &end, 10);
    if (end == p) {
        return false;
    }

    while (isspace((unsigned char)*end)) {
        end++;
    }
    if (*end != ':') {
        return false;
    }

    *plate_num = (int)num;
    *rest = trim(end + 1);
    return true;
}

ParseResult parse_lock_file(FILE *in, LockState *lock) {
    if (!in || !lock) {
        return PARSE_ERR_IO;
    }

    lock_init(lock);

    char line[512];
    bool have_name = false;
    bool in_rules = false;
    int rules_read = 0;
    bool have_start = false;

    while (fgets(line, sizeof(line), in)) {
        char *t = trim(line);
        if (*t == '\0' || *t == '#') {
            continue;
        }

        if (strncmp(t, "Name:", 5) == 0) {
            const char *name = trim(t + 5);
            strncpy(lock->name, name, sizeof(lock->name) - 1);
            lock->name[sizeof(lock->name) - 1] = '\0';
            have_name = true;
            continue;
        }

        if (strcmp(t, "Rules:") == 0) {
            in_rules = true;
            continue;
        }

        if (strncmp(t, "Start:", 6) == 0) {
            in_rules = false;
            char *start_part = trim(t + 6);
            if (*start_part == '\0') {
                if (!fgets(line, sizeof(line), in)) {
                    return PARSE_ERR_FORMAT;
                }
                start_part = trim(line);
            }
            if (!parse_start_array(start_part, lock->start, &lock->n)) {
                return PARSE_ERR_POSITIONS;
            }
            have_start = true;
            break;
        }

        if (in_rules) {
            int plate_num = 0;
            char *rest = NULL;
            if (!parse_rule_header(t, &plate_num, &rest)) {
                return PARSE_ERR_RULES;
            }
            if (plate_num < 1 || plate_num > MAX_PLATES) {
                return PARSE_ERR_RULES;
            }
            if (plate_num != rules_read + 1) {
                return PARSE_ERR_RULES;
            }
            if (!parse_rule_line(rest, rules_read, MAX_PLATES, lock->matrix)) {
                return PARSE_ERR_RULES;
            }
            rules_read++;
        }
    }

    if (!have_start) {
        return PARSE_ERR_FORMAT;
    }

    if (rules_read != 0 && rules_read != lock->n) {
        return PARSE_ERR_PLATES;
    }

    if (rules_read == 0) {
        /* Rules may follow Start in some inputs; read remaining rule lines. */
        while (fgets(line, sizeof(line), in)) {
            char *t = trim(line);
            if (*t == '\0' || *t == '#') {
                continue;
            }
            int plate_num = 0;
            char *rest = NULL;
            if (!parse_rule_header(t, &plate_num, &rest)) {
                break;
            }
            if (plate_num < 1 || plate_num > lock->n || plate_num != rules_read + 1) {
                return PARSE_ERR_RULES;
            }
            if (!parse_rule_line(rest, rules_read, lock->n, lock->matrix)) {
                return PARSE_ERR_RULES;
            }
            rules_read++;
        }
        if (rules_read != lock->n) {
            return PARSE_ERR_PLATES;
        }
    }

    if (!have_name) {
        strcpy(lock->name, "Unnamed lock");
    }

    if (!is_valid_state(lock->start, lock->n)) {
        return PARSE_ERR_POSITIONS;
    }

    return PARSE_OK;
}

ParseResult parse_lock_path(const char *path, LockState *lock) {
    FILE *in = fopen(path, "r");
    if (!in) {
        return PARSE_ERR_IO;
    }
    ParseResult result = parse_lock_file(in, lock);
    fclose(in);
    return result;
}
