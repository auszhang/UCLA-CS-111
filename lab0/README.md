NAME: Christopher Aziz
EMAIL: caziz@ucla.edu
ID: 000000000

# Project/Lab 0: Warm-Up

## Included Files
lab0.c: Source code for lab0

Makefile: Makefile that can build, check, clean, or dist the project

backtrace.png: Screenshot detailing use of gdb to inspect backtrace after use of segfault option

pointer.png: Screenshot detailing use of gdb to inspect NULL pointer

README(this file): Provides information detailing the project

## Smoke-test Cases
The following smoke-test checks are provided by the Makefile.

check_success: Checks that a normal copy of stdin to stdout produces exit code of 0.

check_invalid_option: Checks that an invalid option produces an exit code of 1.

check_invalid_input: Checks that a nonexistent input argument produces an exit code of 2.

check_invalid_output: Checks that an unwritable output argument produces an exit code of 3.

check_segfault_catch: Checks that a caught Segmentation Fault produces an exit code of 4.

## Research
I used gnu.org to learn how to use c library functions such as getopt and how to properly create a Makefile. 
