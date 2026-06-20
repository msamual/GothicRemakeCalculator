#ifndef PARSER_H
#define PARSER_H

#include "lock.h"
#include <stdio.h>

typedef enum {
    PARSE_OK = 0,
    PARSE_ERR_IO,
    PARSE_ERR_FORMAT,
    PARSE_ERR_PLATES,
    PARSE_ERR_POSITIONS,
    PARSE_ERR_RULES
} ParseResult;

ParseResult parse_lock_file(FILE *in, LockState *lock);
ParseResult parse_lock_path(const char *path, LockState *lock);

#endif
