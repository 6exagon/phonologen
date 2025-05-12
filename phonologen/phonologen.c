// Command-line utility to simulate the application of phonological rules to segments
// See project README.txt for details

#include <stdio.h>
#include <stdlib.h>

#include "structures.h"
#include "parsing.h"

// Parses command-line arguments, reads in features .csv and rule order .txt, and does mainloop
// Returns EXIT_SUCCESS on success, EXIT_FAILURE on failure
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: phonologen features-file rules-file\n");
        return EXIT_FAILURE;
    }
    
    // If this fails, program need not error out; we'll just leak memory
    atexit(free_global_structures);
    
    FILE *fp;
    char parse_success;
    
    // Global data structures in structures.h are set here
    // Globals are necessary because there would be too many output parameters, and they'll be
    // used everywhere
    fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "Error opening features .csv file %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    parse_success = parse_features(fp);
    fclose(fp);
    if (!parse_success) {
        // parse_features will print error messages to stderr on its own
        return EXIT_FAILURE;
    }
    
    fp = fopen(argv[2], "rb");
    if (!fp) {
        fprintf(stderr, "Error opening rules .txt file %s\n", argv[2]);
        return EXIT_FAILURE;
    }
    parse_success = parse_rules(fp);
    fclose(fp);
    if (!parse_success) {
        // parse_rules will print error messages to stderr on its own
        return EXIT_FAILURE;
    }
    
    char next_word[256];
    while (scanf("%255s", next_word) != EOF) {
        puts(next_word);
    }
    return EXIT_SUCCESS;
}
