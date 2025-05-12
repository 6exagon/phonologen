#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "structures.h"

uint16_t hash_string(const char *string) {
    // This must be fast, but it does need to hash the entire string for uniqueness; some
    // segments can be long but differ only in a few bits
    register uint32_t result = string[1];
    register unsigned char c;
    while ((c = *string++)) {
        result = (result << 5) ^ c;
    }
    result ^= result >> 16;
    return result;
}

uint16_t hash_feature_matrix(const feature_t fmatrix[]) {
    
}

void free_global_structures() {
    if (g_feature_names) {
        for (unsigned int x = 0; x < g_feature_count; free(g_feature_names[x++]));
        free(g_feature_names);
    }
}
