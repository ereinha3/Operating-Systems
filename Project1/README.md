# Project 1: Command Parser and String Manipulation

This project implements a command parser and string manipulation system in C, focusing on fundamental operating system concepts related to command processing and memory management.

## Project Components

- `main.c`: Main program entry point and command execution logic
- `command.c/h`: Command structure and processing functionality
- `string_parser.c/h`: String parsing and manipulation utilities
- `Makefile`: Build configuration
- `input.txt`: Sample input file for testing

## Features

- Command line parsing and tokenization
- Memory management and leak prevention (verified with Valgrind)
- Support for various command formats and arguments
- Error handling and input validation

## Building and Running

To compile the project:
```bash
make
```

To run the program:
```bash
./main input.txt
```

## Testing and Validation

The implementation has been tested with:
- Various input commands
- Memory leak checking using Valgrind
- Edge cases and error conditions

See `valgrind_screenshot.png` and `output_screenshot.png` for verification of correct execution.

## Documentation

Detailed documentation and implementation details can be found in:
- `project1-description.pdf`: Original project requirements
- `report.pdf`: Project report and analysis
