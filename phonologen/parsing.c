// Utility functions to parse features .csv and rule order .txt files

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "structures.h"
#include "parsing.h"

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
// Returns 1 on success, 0 on failure (and prints to stderr)
// The first row of the .csv file must be an empty cell followed by any number of feature names
static inline char parse_feature_names(FILE *fp) {
    char delimiter = fgetc(fp);
    // Short-circuiting to check these three bytes
    if (delimiter == '\xef' && (char) fgetc(fp) == '\xbb' && (char) fgetc(fp) == '\xbf') {
        // Skip UTF-8 BOM, otherwise let it error out
        delimiter = fgetc(fp);
    }
    // This should probably be compiled to a lookup table
    if (delimiter != ';' && delimiter != '\t' && delimiter != ',') {
        fputs("Error parsing .csv: empty first cell required\n", stderr);
        return 0;
    }
    char line[LINE_LIMIT];
    if (!fgets(line, LINE_LIMIT, fp)) {
        fputs("Error parsing .csv: missing feature names\n", stderr);
        return 0;
    }
    // Pass 1: count delimiters, and add 1, to get number of features
    g_feature_count = 1;
    char *lscan;
    for (lscan = line; *lscan; g_feature_count += (*lscan++ == delimiter));
    // Un-NUL-terminate the string and replace with delimiter
    *lscan = delimiter;
    // Use calloc so it's always safe to free all the entries in this table
    g_feature_names = calloc(g_feature_count, sizeof(*g_feature_names));
    if (!g_feature_names) {
        fputs("Error parsing .csv: unable to allocate memory\n", stderr);
        return 0;
    }
    // Pass 2: lstrip and rstrip feature names, and put tokens in memory
    // strtok not used for this, because it would be harder
    lscan = line;
    char *tokend;
    for (unsigned int x = 0; x < g_feature_count; x++) {
        tokend = strchr(lscan, delimiter);
        // It's impossible that tokend is NULL, because we counted earlier
        lscan = l_r_strip(lscan, tokend);
        // Move tokend past resultant NUL; it may be way past the end of the string due to
        // l_r_strip, but this is fine
        tokend++;
        if (tokend - lscan < 2) {
            fputs("Error parsing .csv: feature names must not be empty\n", stderr);
            return 0;
        }
        g_feature_names[x] = malloc(tokend - lscan);
        if (!g_feature_names[x]) {
            fputs("Error parsing .csv: unable to allocate memory\n", stderr);
            return 0;
        }
        strcpy(g_feature_names[x], lscan);
        lscan = tokend;
    }
    // Build hash table from name to index
    return 1;
}

char parse_features(FILE *fp) {
    if (!parse_feature_names(fp)) {
        return 0;
    }
    char line[LINE_LIMIT];
    return 1;
}

char parse_rules(FILE *fp) {
    return 1;
}
