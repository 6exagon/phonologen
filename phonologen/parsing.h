#ifndef PARSING_H
#define PARSING_H

#include <stdio.h>
#include <stdint.h>

char parse_csv(uint8_t *, uint64_t);
char parse_rules(FILE *);

#endif
