// Command-line utility to simulate the application of phonological rules to segments
// See project README.txt for details

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "parsing.h"
// Unfortunately, these aren't const for some reason
#include "features.h"

// Parses command-line arguments, reads in features .csv and rule order .txt, and does mainloop
// Returns EXIT_SUCCESS on success, EXIT_FAILURE on failure
int main(int argc, char *argv[]) {
    FILE *fp;
    // Start these pointers to the embedded features.h binary data, but they can be moved to a
    // custom feature file
    // It isn't ideal to have both the data and its length for a UTF-8 file, but it's NUL-safe and
    // what xxd outputs
    uint8_t *feature_csv = features_csv;
    uint64_t feature_csv_len = features_csv_len;
    char parse_success;
    switch (argc) {
        default:
            fprintf(stderr, "Usage: phonologen rules-file [features-file]\n");
            return EXIT_FAILURE;
        case 3:
            // This section is a little odd, but we want the code to remain the same between using
            // the embedded features.h binary data and a file
            fp = fopen(argv[2], "rb");
            if (!fp) {
                fprintf(stderr, "Error opening features .csv file %s\n", argv[2]);
                return EXIT_FAILURE;
            }
            fseek(fp, 0, SEEK_END);
            feature_csv_len = ftell(fp);
            rewind(fp);
            feature_csv = malloc(feature_csv_len);
            if (!feature_csv || fread(feature_csv, 1, feature_csv_len, fp) < feature_csv_len) {
                fclose(fp);
                fprintf(stderr, "Error reading features .csv file %s\n", argv[2]);
                return EXIT_FAILURE;
            }
            fclose(fp);
        case 2:
            parse_success = parse_csv(feature_csv, feature_csv_len);
            // Possible if we fall through into here
            if (feature_csv != features_csv) {
                free(feature_csv);
            }
            if (!parse_success) {
                // parse_csv will print error messages to stderr on its own
                return EXIT_FAILURE;
            }
    }
    fp = fopen(argv[1], "rb");
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
