#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "structures.h"
#include "util.h"

unsigned int g_feature_count;
char **g_feature_names;
struct hash_table_node *g_segment_list;
struct hash_table_node *g_feature_lookup_table[HASH_TABLE_SIZE];
struct hash_table_node *g_segment_lookup_table[HASH_TABLE_SIZE];
struct hash_table_node *g_fmatrix_cache[HASH_TABLE_SIZE];

uint16_t hash_string(const char *string) {
    // This must be fast, but it does need to hash the entire string for uniqueness; some segments
    // can be long but differ only in a few bits
    // We assume strlen(string) >= 1
    // Not an extremely good hash algorithm, but better ones are slightly slower
    register uint32_t result = string[1];
    register unsigned char c;
    while ((c = *string++)) {
        result = (result << 3) ^ c;
    }
    result ^= result >> 16;
    return result;
}

uint16_t hash_fmatrix(const feature_t fmatrix[]) {
    // This must be fast, but it does need to hash the entire matrix for uniqueness; some matrices
    // can differ only in a few bits
    // Not an extremely good hash algorithm, but better ones are slightly slower
    register uint32_t result = fmatrix[1];
    register feature_t c;
    while ((c = *fmatrix++)) {
        // Only two bits per value are relevant
        result = ((result << 1) + result) ^ c;
    }
    result ^= result >> 16;
    return result;
}

void linked_list_strkey_add(struct hash_table_node **list_ptr, const char *key, const void *value) {
    // next_ptr is indirect so malloc can be used to set start of list and end of it
    while (*list_ptr) {
        struct hash_table_node *node = *list_ptr;
        fail_if(!strcmp(node->key, key), "Error parsing .csv: duplicate string %s\n", key);
        list_ptr = &(node->next);
    }
    // Make a new node and add the key and value to it
    *list_ptr = malloc(sizeof(**list_ptr));
    fail_if(!*list_ptr, "Error parsing .csv: unable to allocate memory\n");
    (*list_ptr)->next = NULL;
    (*list_ptr)->key = key;
    (*list_ptr)->value = value;
}

void hash_table_strkey_add(struct hash_table_node *table[], const char *key, const void *value) {
    // kh can be used as the bucket directly due to our 64k hash tables
    uint16_t kh = hash_string(key);
    linked_list_strkey_add(table + kh, key, value);
}

void fmatrix_cache_add(const feature_t key[], const char *value) {
    // kh can be used as the bucket directly due to our 64k hash tables
    uint16_t kh = hash_fmatrix(key);
    // next_ptr is indirect so malloc can be used to set start of hash table list and end of it
    struct hash_table_node **next_ptr = g_fmatrix_cache + kh;
    while (*next_ptr) {
        struct hash_table_node *node = *next_ptr;
        fail_if(fmatrix_compare(node->key, key) == EQUAL, "Error: duplicate feature matrix");
        next_ptr = &(node->next);
    }
    // Make a new node and add the key and value to it
    *next_ptr = malloc(sizeof(**next_ptr));
    fail_if(!*next_ptr, "Error parsing .csv: unable to allocate memory\n");
    (*next_ptr)->next = NULL;
    (*next_ptr)->key = key;
    (*next_ptr)->value = value;
}

const void *hash_table_strkey_find(struct hash_table_node *table[], const char *key) {
    // kh can be used as the bucket directly due to our 64k hash tables
    uint16_t kh = hash_string(key);
    struct hash_table_node *bucket = table[kh];
    while (bucket) {
        if (!strcmp(bucket->key, key)) {
            return bucket->value;
        }
        bucket = bucket->next;
    }
    // Always fail, we didn't find the string which should never happen
    fail_if(1, "Error: unknown string %s\n", key);
    // Unreachable, needed to silence compiler warnings
    return NULL;
}

const char *fmatrix_cache_find(const feature_t key[]) {
    // kh can be used as the bucket directly due to our 64k hash tables
    uint16_t kh = hash_fmatrix(key);
    struct hash_table_node *bucket = g_fmatrix_cache[kh];
    while (bucket) {
        if (fmatrix_compare(bucket->key, key) == EQUAL) {
            return bucket->value;
        }
        bucket = bucket->next;
    }
    return NULL;
}

enum set_relation fmatrix_compare(const feature_t a[], const feature_t b[]) {
    // They could be equal at the start
    char is_super = 1;
    char is_sub = 1;
    for (register unsigned int f = 0; f < g_feature_count; f++) {
        // For optimal performance, verify that (on x86) this compiles to movb into the high and
        // low byte registers (h and l) of a larger register, and then use a 514-pointer jump table
        // Inline assembly not used for compatibility
        register uint16_t sv = (a[f] << 8) + b[f];
        switch (sv) {
            //    af           bf
            case (ZERO << 8) + ZERO:
            case (PLUS << 8) + PLUS:
            case (MINUS << 8) + MINUS:
                // They're the same; no change
                break;
            case (ZERO << 8) + PLUS:
            case (ZERO << 8) + MINUS:
                // a could no longer be a subset of b
                is_sub = 0;
                if (!is_super) {
                    return NONE;
                }
                break;
            case (PLUS << 8) + ZERO:
            case (MINUS << 8) + ZERO:
                is_super = 0;
                if (!is_sub) {
                    return NONE;
                }
                break;
            case (PLUS << 8) + MINUS:
            case (MINUS << 8) + PLUS:
                return NONE;
        }
    }
    const enum set_relation outcomes[] = {NONE, SUBSET, SUPERSET, EQUAL};
    return outcomes[is_super * 2 + is_sub];
}

void fmatrix_print(const feature_t fmatrix[], char include_names) {
    fputs("[ ", stdout);
    const char mappings[] = {'0', '+', '-'};
    for (register unsigned int f = 0; f < g_feature_count; f++) {
        putchar(mappings[fmatrix[f]]);
        if (include_names) {
            fputs(g_feature_names[f], stdout);
        }
        putchar(' ');
    }
    putchar(']');
}

// Frees the nodes in one single linked list (though doesn't free the list itself; this is static)
// If free_keys, keys will be freed as well
static inline void free_linked_list(struct hash_table_node *list, char free_keys) {
    while (list) {
        struct hash_table_node *next = list->next;
        if (free_keys) {
            // Discard the const qualifier, we're freeing this
            free((void *) (list->key));
        }
        free(list);
        list = next;
    }
}

// Frees the nodes in one single hash table (though doesn't free the table itself; these are static)
// If free_keys, keys will be freed as well
static inline void free_hash_table(struct hash_table_node *table[], char free_keys) {
    uint16_t index = 0;
    do {
        free_linked_list(table[index], free_keys);
    } while (--index);
}

void free_global_structures() {
    // The freeing of its contents is below
    free_hash_table(g_feature_lookup_table, 0);
    if (g_feature_names) {
        for (unsigned int x = 0; x < g_feature_count; free(g_feature_names[x++]));
        free(g_feature_names);
    }
    free_hash_table(g_segment_lookup_table, 0);
    // This table is where the keys of the above table (its inverse) are freed
    free_hash_table(g_fmatrix_cache, 1);
    // Note that all the values (feature matrices) have already been freed! Do not touch these
    // The segment names are all freed here
    free_linked_list(g_segment_list, 1);
}
