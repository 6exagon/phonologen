// Command-line utility to simulate the application of phonological rules to segments
// See project README.txt for details

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structures.h"
#include "parsing.h"
#include "util.h"

// Returns whether rule matches (applies) to the feature matrices given by environment
// Assumes environment points to an array of pointers exactly r->context_length long
// Thus, this must be called a number of times proportional to the length of the input string per
// rule application
static inline char rule_matches(const struct rule *r, feature_t *const *environment) {
    for (register short x = 0; x < r->context_length; x++) {
        enum set_relation sr = fmatrix_compare(r->context[x], environment[x]);
        // Exact match is good; superset is also good (the rule is a superset of the de facto
        // environment)
        if (sr == NONE || sr == SUBSET) {
            return 0;
        }
    }
    return 1;
}

// Applies mask fmatrix to changed fmatrix (for every PLUS or MINUS in mask, sets that in changed)
static inline void fmatrix_apply(const feature_t mask[], feature_t *changed) {
    for (register unsigned int f = 0; f < g_feature_count; f++) {
        if (mask[f] == PLUS) {
            changed[f] = PLUS;
        } else if (mask[f] == MINUS) {
            changed[f] = MINUS;
        }
    }
}

// Parses command-line arguments, reads in features .csv and rule order .txt, and does mainloop
// Returns EXIT_SUCCESS on success, EXIT_FAILURE on failure
int main(int argc, char *argv[]) {
    fail_if(argc != 3, "Usage: phonologen features-file rules-file\n");

    // If this fails, program need not error out; we'll just leak memory
    atexit(free_global_structures);

    FILE *fp;
    char parse_success;

    // Global data structures in structures.h are set here
    // Globals are necessary because there would be too many output parameters, and they'll be
    // used everywhere
    fp = fopen(argv[1], "rb");
    fail_if(!fp, "Error opening features .csv file %s\n", argv[1]);
    parse_features(fp);
    fclose(fp);

    fp = fopen(argv[2], "rb");
    fail_if(!fp, "Error opening rules .txt file %s\n", argv[2]);
    parse_rules(fp);
    fclose(fp);

    char next_word[256];
    while (scanf("%255s", next_word) != EOF) {
        long len;
        // This tokenizes next_word, messing it up
        feature_t **next_word_fmatrices = parse_word(next_word, &len);
        for (struct rule *r = g_rules; r; r = r->next) {
            // Use long rather than size_t so it can become negative
            if (r->direction == 'L') {
                for (long x = 0; x <= len - r->context_length; x++) {
                    if (rule_matches(r, next_word_fmatrices + x)) {
                        fmatrix_apply(r->output, next_word_fmatrices[x + r->focus_position]);
                    }
                }
            } else {
                // r->direction == 'R'
                for (long x = len - r->context_length; x >= 0; x--) {
                    if (rule_matches(r, next_word_fmatrices + x)) {
                        fmatrix_apply(r->output, next_word_fmatrices[x + r->focus_position]);
                    }
                }
            }
        }
        for (size_t x = 0; x < len; x++) {
            fputs(fmatrix_cache_find(next_word_fmatrices[x]), stdout);
            free(next_word_fmatrices[x]);
        }
        putchar(' ');
        free(next_word_fmatrices);
    }
    return EXIT_SUCCESS;
}
