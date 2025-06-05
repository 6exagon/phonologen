// Utility functions to parse features .csv and rule order .txt files

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "structures.h"
#include "parsing.h"
#include "util.h"

#define LINE_LIMIT 2048

static const char DELIMS[] = " \t\n";

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

// Parses one segment, either a string from the features.csv file or a feature matrix directly
// Returns NULL if an underscore is found instead of a segment
// line_number used for printing error messages
// MAY CALL strtok AGAIN! This means it's only used within parse_rule, and carefully!
static inline feature_t *parse_segment(char *token, unsigned int line_number) {
    if (*token == '_') {
        // Assume this is one or more underscores, don't worry about strange exceptions
        return NULL;
    }
    // Start all features at ZERO
    feature_t *new_fmatrix = calloc(g_feature_count, sizeof(*new_fmatrix));
    fail_if(!new_fmatrix, "Error parsing .txt: unable to allocate memory\n");
    if (*token == '[') {
        fail_if(
            strlen(token) > 1,
            "Error parsing .txt: malformed feature matrix on line %u\n",
            line_number);
        while ((token = strtok(NULL, DELIMS))) {
            feature_t value = 0;
            switch (*token) {
                case '[':
                    fail_if(1, "Error parsing .txt: two open braces on line %u\n", line_number);
                case ']':
                    fail_if(
                        strlen(token) > 1,
                        "Error parsing .txt: malformed feature matrix on line %u\n",
                        line_number);
                    // We hit a '[', read all the values and feature names, and hit a ']', so done
                    return new_fmatrix;
                case '0':
                    // We shouldn't allow this, just because that's how it is by default
                    fail_if(1, "Error parsing .txt: zero-valued feature on line %u\n", line_number);
                default:
                    fail_if(1, "Error parsing .txt: no feature value on line %u\n", line_number);
                case '+':
                    value = PLUS;
                    break;
                case '-':
                    value = MINUS;
            }
            unsigned int index = (unsigned int) hash_table_strkey_find(
                g_feature_lookup_table, token + 1);
            new_fmatrix[index] = value;
        }
        // We got an unexpected NULL before the closing ']'
        fail_if(1, "Error parsing .txt: unclosed feature matrix on line %u\n", line_number);
    } else {
        // Copy into new_fmatrix what the features for token are
        memcpy(
            new_fmatrix,
            hash_table_strkey_find(g_segment_lookup_table, token),
            g_feature_count * sizeof(*new_fmatrix));
    }
    // Unreachable from if-branch, but that's ok
    return new_fmatrix;
}

// Parses one line of rules UTF-8 .txt file
// Prints errors to stderr and exits on failure (this is what line_number is used for)
// Format: D P > P / P P ... _ P P ...
// Where D is either L (for left-to-right application) or R (for the opposite) and P is either a
// segment defined in the features .csv file or a feature matrix in the format [ +f -f 0f ... ]
// Returns pointer to new heap-allocated struct rule representing the rule for the current line
static inline struct rule *parse_rule(char *line, unsigned int line_number) {
    struct rule *new = malloc(sizeof(*new));
    fail_if(!new, "Error parsing .txt: unable to allocate memory\n");
    new->next = NULL;
    char *tok;
    // First, the direction
    tok = strtok(line, DELIMS);
    fail_if(
        // "Left" and "Right" are also okay
        !tok || (*tok != 'L' && *tok != 'R'),
        "Error parsing .txt: invalid direction on line %u\n",
        line_number);
    new->direction = *tok;
    // Then, the input
    tok = strtok(NULL, DELIMS);
    fail_if(!tok, "Error parsing .txt: incomplete line %u\n", line_number);
    feature_t *focus = parse_segment(tok, line_number);
    fail_if(!focus, "Error parsing .txt: _ as input on line %u\n", line_number);
    // >
    tok = strtok(NULL, DELIMS);
    fail_if(!tok || strcmp(tok, ">"), "Error parsing .txt: expected '>' on line %u\n", line_number);
    // Then, the output
    tok = strtok(NULL, DELIMS);
    fail_if(!tok, "Error parsing .txt: incomplete line %u\n", line_number);
    new->output = parse_segment(tok, line_number);
    fail_if(!new->output, "Error parsing .txt: _ as output on line %u\n", line_number);
    // /
    tok = strtok(NULL, DELIMS);
    fail_if(!tok || strcmp(tok, "/"), "Error parsing .txt: expected '/' on line %u\n", line_number);
    // Set this to an invalid value
    new->focus_position = -1;
    short context_length = 0;
    while ((tok = strtok(NULL, DELIMS))) {
        fail_if(
            context_length >= MAX_CONTEXT_LENGTH,
            "Error parsing .txt: context too long on line %u\n",
            line_number);
        feature_t *contextfm = parse_segment(tok, line_number);
        if (contextfm) {
            new->context[context_length++] = contextfm;
        } else {
            fail_if(
                new->focus_position >= 0,
                "Error parsing .txt: multiple _ on line %u\n",
                line_number);
            new->focus_position = context_length;
            new->context[context_length++] = focus;
        }
    }
    new->context_length = context_length;
    fail_if(
        new->focus_position == -1,
        "Error parsing .txt: no _ on line %u\n",
        line_number);
    return new;
}

void parse_rules(FILE *fp) {
    char line[LINE_LIMIT];
    fail_if(!fgets(line, LINE_LIMIT, fp), "Error parsing .txt: empty file");
    struct rule *tail = parse_rule(line, 1);
    g_rules = tail;
    unsigned int line_number = 2;
    while (fgets(line, LINE_LIMIT, fp)) {
        struct rule *new = parse_rule(line, line_number);
        tail->next = new;
        tail = new;
        line_number++;
    }
}

feature_t **parse_word(char *word, size_t *output_len) {
    register size_t segments = 1;
    for (char *c = word; *c; c++) {
        segments += (*c == '.');
    }
    *output_len = segments;
    feature_t **output = malloc(segments * sizeof(*output));
    fail_if(!output, "Error parsing word: unable to allocate memory\n");
    segments = 0;
    for (char *tok = strtok(word, "."); tok; tok = strtok(NULL, ".")) {
        // We need copies of existing feature matrices; these need to be modified by rules
        feature_t *copy = malloc(g_feature_count * sizeof(*copy));
        fail_if(!copy, "Error parsing word: unable to allocate memory\n");
        // Copy into copy what the features for token are
        memcpy(
            copy,
            hash_table_strkey_find(g_segment_lookup_table, tok),
            g_feature_count * sizeof(*copy));
        output[segments] = copy;
        segments++;
    }
    return output;
}
