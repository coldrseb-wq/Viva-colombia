#ifndef RUNTIME_H
#define RUNTIME_H

// Built-in functions for the Viva language
int builtin_print(char* str);
int builtin_println(char* str);
void builtin_print_num(int num);
void builtin_println_num(int num);
int builtin_strlen(char* str);
int builtin_int(char* str);
void builtin_exit(int code);

// Colombian hero functions
int builtin_simon_bolivar();
int builtin_francisco_narino();
int builtin_maria_cano();
int builtin_jorge_eliecer_gaitan();
int builtin_gabriel_garcia_marquez();

#endif