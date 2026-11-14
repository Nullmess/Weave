#pragma once
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

/* Case-insensitive filename substring search, ASCII-safe, no heap allocation. */
static inline bool archive_search_match(const char* name, const char* query) {
    if(!name || !query || !*query) return false;
    for(; *name; ++name) {
        const char* n = name;
        const char* q = query;
        while(*n && *q && tolower((unsigned char)*n) == tolower((unsigned char)*q)) {
            ++n;
            ++q;
        }
        if(!*q) return true;
    }
    return false;
}
