#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "structures.h"

uint16_t hash_string(const char *string) {
    // This must be fast, but it does need to hash the entire string for uniqueness; some
    // segments can be long but differ only in a few bits
    // We assume strlen(string) >= 1
    register uint32_t result = string[1];
    register unsigned char c;
    while ((c = *string++)) {
        result = (result << 3) ^ c;
    }
    result ^= result >> 16;
    return result;
}

uint16_t hash_feature_matrix(const feature_t fmatrix[]) {

}

void feature_lookup_table_add(char *key, uint16_t index) {
    // kh can be used as the bucket directly due to our 64k hash tables
    uint16_t kh = hash_string(key);
    // next_ptr is indirect so malloc can be used to set start of hash table list and end of it
    struct hash_table_node **next_ptr = g_feature_lookup_table + kh;
    while (*next_ptr) {
        struct hash_table_node *node = *next_ptr;
        if (!strcmp((*next_ptr)->key, key)) {
            fprintf(stderr, "Error parsing .csv: duplicate feature %s\n", key);
            exit(EXIT_FAILURE);
        }
        next_ptr = &(node->next);
    }
    // Make a new node and add the key and value to it
    *next_ptr = malloc(sizeof(**next_ptr));
    if (!*next_ptr) {
        fputs("Error parsing .csv: unable to allocate memory\n", stderr);
        exit(EXIT_FAILURE);
    }
    (*next_ptr)->next = NULL;
    (*next_ptr)->key = key;
    // This cast is not ideal, but we store an int type as a void pointer
    (*next_ptr)->value = (void *) (uintptr_t) index;
}

uint16_t feature_lookup_table_find(char *key) {
    // kh can be used as the bucket directly due to our 64k hash tables
    uint16_t kh = hash_string(key);
    struct hash_table_node *bucket = g_feature_lookup_table[kh];
    while (bucket) {
        if (!strcmp(bucket->key, key)) {
            return (uint16_t) bucket->value;
        }
        bucket = bucket->next;
    }
    fprintf(stderr, "Error TODO: unknown feature %s\n", key);
    exit(EXIT_FAILURE);
}

void free_global_structures() {
    uint16_t index = 0;
    do {
        struct hash_table_node *bucket = g_feature_lookup_table[index];
        while (bucket) {
            struct hash_table_node *next = bucket->next;
            free(bucket);
            bucket = next;
        }
    } while (--index);
    if (g_feature_names) {
        for (unsigned int x = 0; x < g_feature_count; free(g_feature_names[x++]));
        free(g_feature_names);
    }
}
