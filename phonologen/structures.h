#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdint.h>

// This is needed because hash functions output uint16_t
#define HASH_TABLE_SIZE (UINT16_MAX + 1)
// This will make our struct rule 128 bytes long including padding
#define MAX_CONTEXT_LENGTH 13

// This is unfortunately required for one-byte enums
typedef char feature_t;
// Feature 0 is 0 for faster comparisons
// The redefinition is required so the lower two bits are the only significant ones
enum feature {
    ZERO, PLUS, MINUS
};
// Note that feature_t [] (feature matrices) must be a zero-padded to 16-byte alignment normally

enum set_relation {
    NONE, EQUAL, SUPERSET, SUBSET
};

// Node of a hash-table-internal linked list
// Also used by g_segment_list
// Note that key and value are not usually owned by the hash tables; they must usually be allocated
// and freed separately
// Also, key and value are unfortunately represented as void *
struct hash_table_node {
    struct hash_table_node *next;
    const void *key;
    const void *value;
};
// Phonological rule, with pointer to the next rule that should be applied
struct rule {
    // An array of feature matrices for surrounding context for the input, including the input itself
    feature_t *context[MAX_CONTEXT_LENGTH];
    // What gets changed about input (here, any nonzero values are set in the focus)
    feature_t *output;
    struct rule *next;
    short context_length;
    // Which position of the context the input is (so rather than A > B / C _ D, > B / C _A_ D in
    // representation)
    short focus_position;
    // Either L for left-to-right, or R for right-to-left
    char direction;
};

// All global data structures zero- and NULL-initialized by default
// All hash tables are arrays HASH_TABLE_SIZE big of pointers to heads of linked lists
// Note that hash tables need no initialization
extern unsigned int g_feature_count;
// Array of char *s, each pointing to one heap-allocated feature name
// Owns all the feature names
extern char **g_feature_names;
// We unfortunately can't use a decision tree to store nodes in a way that would make it easy to
// see related feature matrices, so instead we'll use a linked list of segment names and their
// feature matrices and check them all by going through it
// Owns all the segment names, though it does not own all the feature matrices
extern struct hash_table_node *g_segment_list;
// All hash table keys and values are reinterpreted as void * for the table
// Hash table mapping feature names back to indices in the g_feature_names list
// Keys: char *; Values: size_t
// Does not own anything
extern struct hash_table_node *g_feature_lookup_table[HASH_TABLE_SIZE];
// Hash table mapping segments to feature matrices
// Keys: char *; Values: feature_t []
// Does not own anything
extern struct hash_table_node *g_segment_lookup_table[HASH_TABLE_SIZE];
// Hash table mapping feature matrices to segments
// Keys: feature_t [], Values: char *
// Inverse of the above table
// Acts as a cache to store every feature matrix that gets searched for (only when converting
// from feature matrices to segments in the output text)
// Note that any feature matrix that is a subset of a segment we have will be displayed as that
// segment (for example, specifying one of a basic phone like [p]'s 0 features)
// This add-only cache is fine since probably no more than several hundred symbols need printing
// Owns all of the feature matrices, though it does not own all the segments
extern struct hash_table_node *g_fmatrix_cache[HASH_TABLE_SIZE];
// Linked list of all phonological rules to be applied, in order
// Owns all of the data structures within it
extern struct rule *g_rules;

// Function for hashing strings, prioritizing speed
// Assumes string is not empty or NULL
uint16_t hash_string(const char *);
// Function for hashing feature matrices, keeping in mind that they're strings of 0, 1, and 2
// Does not bounds-check feature matrix, array must be g_feature_count long
uint16_t hash_fmatrix(const feature_t []);
// Adds key-value pair to linked list, where key is a string
// Causes error on duplicates (there should be none for everything that uses this function)
// This is called within a hash table, but also on its own
// The reason the first parameter is indirect is so it can modify variables in-place
void linked_list_strkey_add(struct hash_table_node **, const char *, const void *);
// Adds key-value pair to hash table, where key is a string
// Causes error on duplicates (there should be none for both of the applicable hash tables)
void hash_table_strkey_add(struct hash_table_node *[], const char *, const void *);
// Adds key-value pair to g_fmatrix_cache
// Causes error on duplicates (there should be none)
void fmatrix_cache_add(const feature_t [], const char *);
// Finds hash table the value pointed to by key
// Causes error on value not found (nonexistent features)
const void *hash_table_strkey_find(struct hash_table_node *[], const char *);
// Finds in g_fmatrix_cache the char * mapped to a feature matrix
// Returns NULL if matrix isn't found
const char *fmatrix_cache_find(const feature_t []);

// Returns how first feature matrix relates to second feature matrix
// Does not bounds-check feature matrices; note that their size must be g_feature_count
enum set_relation fmatrix_compare(const feature_t [], const feature_t []);

// FOR DEBUGGING PURPOSES(?)
// Prints a feature matrix in [valuename valuename ...] format to stdout
// name will be omitted if include_names is false
// Does not bounds-check feature matrices; note that their size must be g_feature_count
void fmatrix_print(const feature_t [], char);
// Prints a rule to stdout followed by a newline
// Recursively prints any rules attached to it afterwards, also followed by newlines
// If rule is none, prints "END\n"
void rule_print(struct rule *);

// Frees all global data structures' allocated data before exit
void free_global_structures(void);

#endif
