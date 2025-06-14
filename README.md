# Phonologen

A program to simulate the application of phonological rules to segments defined by features.

## Setup

#### Requirements

This project has no third-party dependencies, and the code is all cross-platform standard C99. It should run almost anywhere.

#### Compilation

This step will soon be optional for anyone who does not wish to build this project from source; standalone binaries will provided for Windows and macOS in the future. Download and decompress this project's .zip file or clone its source tree, then `cd` into the main directory and run `make`. The program executable should then be created in the `build` directory, and it can be moved to `/usr/local/bin` or anywhere in `$PATH` if desired.

## Use

#### Overview

This program simulates the application of phonological rules to segments defined by features in generative phonology. it allows specification of a feature matrix for segments, which will allow any number of features with arbitrary names and values for symbols to be used (thus permitting archiphonemes, which do work correctly as underspecified segments), and rules with directionality in a specified order. Rules can be feature-matrix based or use segments as shortcuts, or a mix. The program parses all of the above given as input files, listens on stdin for underlying representations of words, and outputs surface forms to stdout. Use of UTF-8 IPA characters for segments is encouraged.

#### Basic Usage

First, use the [included feature chart](features.csv) (from [Bruce Hayes' website](https://brucehayes.org/120a/Features.xlsx), reformatted and exported as a .csv file), or create your own with the same structure. An arbitrary number of segments and feature names can be included. Archiphonemes can also be included if desired (underspecified segments). Next, give any number of rules in the form 'D F `>` O `/` C\* `_` C\*' separated by newlines in a .txt file, where 'D' is the rule directionality (one of `L` or `R` for left-to-right and right-to-left, respectively), and 'F', 'O', and 'C' are the focus, output, and context of the rule, are either segments as defined in the feature chart or feature matrices similar to those found in the [example rules](example_rules.txt) file, using features defined in the chart. These rules will be applied in order to every word separately (for now).

Finally, run the program with these two files as arguments, optionally piping stdin and stdout from and to files. This program was designed to allow large text files to be piped into the program batchwise with a script, and the outputs to be piped to new files, possibly for comparison with an expected output.

#### Example Run

```
$ build/phonologen features.csv example_rules.txt
ioupi uku it
iuubi ugu it
```

The example rules given are slightly naturalistic individually though nonsensical as a complete set of rules, but they can be replaced by similar rules with a context length under 14.

#### Advanced Behaviors

This is still a work in progress, but there will be more to come.

## Contributions

#### Suggestions

Anyone testing the code and reporting bugs or suggesting improvements in the issue tracker is very welcome!
