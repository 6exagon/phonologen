#ifndef PARSING_H
#define PARSING_H

#include <stdio.h>

// Parses features UTF-8 .csv file from FILE *
// Prints errors to stderr and exits on failure
// The first row of the .csv file must be an empty cell followed by any number of feature names
// Every subsequent row must be a phonetic segment followed by '+', '-', or '0' for each feature
// ' ' and 'z' also accepted for 0, 'p' for +, and 'm' for -
// Uppercase capital letters (denoting archiphonemes) are the only phonetic segments where it is
// permissible to have overlap in description with other sounds in the table
void parse_features(FILE *);
// Parses rules UTF-8 .txt file from FILE *
// Prints errors to stderr and exits on failure
void parse_rules(FILE *);

#endif
