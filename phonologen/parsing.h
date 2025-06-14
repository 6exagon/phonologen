#ifndef PARSING_H
#define PARSING_H

#include <stdio.h>

#include "structures.h"

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
// Format: D P > P / P P ... _ P P ...
// Where D is either L (for left-to-right application) or R (for the opposite) and P is either a
// segment defined in the features .csv file or a feature matrix in the format [ +f -f 0f ... ]
void parse_rules(FILE *);
// Parses UTF-8 word, where segments are adjacent to each other (parses segments greedily, which
// may lead to unexpected outcomes in case of ambiguity)
// Outputs dynamically allocated feature matrix array of the proper size (one feature matrix per
// segment in the word)
// Writes the size of this new array to the output_len parameter
// This is a long rather than size_t for subtraction to yield a negative
// Prints errors to stderr and exits on failure
// Assumes input word is not NULL and not an empty string
feature_t **parse_word(char *, long *);

#endif
