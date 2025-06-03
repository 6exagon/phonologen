// Utility functions to parse features .csv and rule order .txt files

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "structures.h"
#include "parsing.h"
#include "util.h"

#define LINE_LIMIT 2048

// Moves char *s towards the middle of a string, until they're no longer whitespace
// Overwrites new end of string with NUL, returns left side of string
static inline char *l_r_strip(char *l, char *r) {
    // Short-circuiting allows this to work properly
    while (isspace(*l) && l++ < r);
    while (isspace(*--r) && l < r);
    *++r = 0;
    return l;
}

// Parses first row of features UTF-8 .csv file from FILE *
// The first row of the .csv file must be an empty cell followed by any number of feature names
static inline void parse_feature_names(FILE *fp, char delimiter) {
    char line[LINE_LIMIT];
    fail_if(!fgets(line, LINE_LIMIT, fp), "Error parsing .csv: missing feature names\n");
    // Pass 1: count delimiters, and add 1, to get number of features
    g_feature_count = 1;
    char *lscan;
    for (lscan = line; *lscan; g_feature_count += (*lscan++ == delimiter));
    // Un-NUL-terminate the string and replace with delimiter
    *lscan = delimiter;
    // Use calloc so it's always safe to free all the entries in this table
    g_feature_names = calloc(g_feature_count, sizeof(*g_feature_names));
    fail_if(!g_feature_names, "Error parsing .csv: unable to allocate memory\n");
    // Pass 2: lstrip and rstrip feature names, and put tokens in memory
    // strtok not used for this, because it would be harder
    lscan = line;
    for (unsigned int x = 0; x < g_feature_count; x++) {
        char *tokend = strchr(lscan, delimiter);
        // It's impossible that tokend is NULL, because we counted earlier
        lscan = l_r_strip(lscan, tokend);
        // Move tokend past resultant NUL; it may be way past the end of the string due to
        // l_r_strip, but this is fine
        tokend++;
        fail_if(tokend - lscan < 2, "Error parsing .csv: feature names must not be empty\n");
        g_feature_names[x] = malloc(tokend - lscan + 1);
        fail_if(!g_feature_names[x], "Error parsing .csv: unable to allocate memory\n");
        strcpy(g_feature_names[x], lscan);
        lscan = tokend;
        // The casts aren't ideal, but it's a good idea here to use void * as the value type
        hash_table_strkey_add(g_feature_lookup_table, g_feature_names[x], (void *) (uintptr_t) x);
    }
}

void parse_features(FILE *fp) {
    char delimiter = fgetc(fp);
    // Short-circuiting to check these three bytes
    if (delimiter == '\xef' && (char) fgetc(fp) == '\xbb' && (char) fgetc(fp) == '\xbf') {
        // Skip UTF-8 BOM, otherwise let it error out
        delimiter = fgetc(fp);
    }
    // This should probably be compiled to a lookup table
    fail_if(
        delimiter != ';' && delimiter != '\t' && delimiter != ',',
        "Error parsing .csv: empty first cell required\n");
    // This will advance FP so we can parse the table afterward
    parse_feature_names(fp, delimiter);
    // Parse table by getting segment, allocating a string for it
    unsigned int line_number = 2;
    char line[LINE_LIMIT];
    while (fgets(line, LINE_LIMIT, fp)) {
        char *tokend = strchr(line, delimiter);
        fail_if(!tokend, "Error parsing .csv: malformed line %u\n", line_number);
        char *feature_value_reader = tokend;
        char *lscan = l_r_strip(line, tokend);
        char *segment_name = malloc(tokend - lscan + 1);
        feature_t *new_fmatrix = malloc(g_feature_count * sizeof(*new_fmatrix));
        fail_if(!segment_name || !new_fmatrix, "Error parsing .csv: unable to allocate memory\n");
        strcpy(segment_name, lscan);
        // We put a NUL where we now expect a delimiter maybe
        *tokend = delimiter;
        // We should see delimiter, value, delimiter, value... for every feature exactly
        for (unsigned int x = 0; x < g_feature_count; x++) {
            fail_if(
                *feature_value_reader != delimiter,
                "Error parsing .csv: unexpected character '%c' on line %u\n",
                *feature_value_reader,
                line_number);
            switch(*++feature_value_reader) {
                case '0':
                case ' ':
                case 'z':
                    new_fmatrix[x] = ZERO;
                    break;
                case '+':
                case 'p':
                    new_fmatrix[x] = PLUS;
                    break;
                case '-':
                case 'm':
                    new_fmatrix[x] = MINUS;
                    break;
                case '\n':
                case '\r':
                case 0:
                    fail_if(
                        1,
                        "Error parsing .csv: not enough feature values on line %u\n",
                        line_number);
                    // Unreachable, but just in case
                    break;
                default:
                    // We shouldn't allow empty cells to count for zero,
                    // that seems error prone
                    fail_if(
                        *feature_value_reader == delimiter,
                        "Error parsing .csv: empty feature on line %u\n",
                        line_number);
                    fail_if(
                        1,
                        "Error parsing .csv: invalid feature value '%c' on line %u\n",
                        *feature_value_reader,
                        line_number);
            }
            feature_value_reader++;
        }
        if (*feature_value_reader == delimiter) {
            feature_value_reader++;
        }
        fail_if(
            *feature_value_reader != '\n' && *feature_value_reader != '\r' && *feature_value_reader,
            "Error parsing .csv: too many features on line %u\n",
            line_number);
        hash_table_strkey_add(g_segment_lookup_table, segment_name, new_fmatrix);
        // This one will be the owner of new_fmatrix
        fmatrix_cache_add(new_fmatrix, segment_name);
        // This one will be the owner of segment_name
        linked_list_strkey_add(&g_segment_list, segment_name, new_fmatrix);
        line_number++;
    }
}

void parse_rules(FILE *fp) {
}
