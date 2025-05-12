#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdint.h>

// This is needed because hash functions output uint16_t
#define HASH_TABLE_SIZE (UINT16_MAX + 1)

// This is unfortunately required for one-byte enums
typedef char feature_t;
// Feature 0 is 0 for faster comparisons
enum feature {
    ZERO, PLUS, MINUS
};

// Node of a hash-table-internal linked list
struct hash_table_node {
    struct hash_table_node *next;
    void *key;
    void *value;
};

// All global data structures zero- and NULL-initialized by default
// All hash tables are arrays HASH_TABLE_SIZE big of pointers to heads of linked lists
// Note that hash tables need no initialization
unsigned int g_feature_count;
// List of char *s, each pointing to one heap-allocated feature name
char **g_feature_names;
// Hash table mapping feature names back to numbers
struct hash_table_node *g_feature_lookup[HASH_TABLE_SIZE];

// Function for hashing strings, prioritizing speed
// Assumes string is not empty or NULL
uint16_t hash_string(const char *);
// Function for hashing feature matrices, keeping in mind that they can
uint16_t hash_feature_matrix(const feature_t []);
// Frees all global data structures' allocated data before exit
void free_global_structures(void);

#endif
