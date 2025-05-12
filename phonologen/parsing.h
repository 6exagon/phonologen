#ifndef PARSING_H
#define PARSING_H

#include <stdio.h>

// Parses features UTF-8 .csv file from FILE *
// Returns 1 on success, 0 on failure (and prints to stderr)
// The first row of the .csv file must be an empty cell followed by any number of feature names
// Every subsequent row must be a phonetic segment followed by '+', '-', or '0' for each feature
// Uppercase capital letters (denoting archiphonemes) are the only phonetic segments where it is
// permissible to have overlap in description with other sounds in the table
char parse_features(FILE *);
// Parses rules UTF-8 .txt file from FILE *
// Returns 1 on success, 0 on failure (and prints to stderr)
char parse_rules(FILE *);

#endif
