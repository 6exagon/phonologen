// Command-line utility to simulate the application of phonological rules to segments
// See project README.txt for details

#include <stdio.h>
#include <stdlib.h>

#include "structures.h"
#include "parsing.h"
#include "util.h"

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
        fmatrix_print(hash_table_strkey_find(g_segment_lookup_table, next_word), 1);
        putchar('\n');
    }
    return EXIT_SUCCESS;
}
