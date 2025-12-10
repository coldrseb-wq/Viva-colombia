#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>

// Built-in print function
int builtin_print(char* str) {
    printf("%s", str);
    return 0;
}

// Built-in print with newline function
int builtin_println(char* str) {
    printf("%s\n", str);
    return 0;
}

// Built-in string length
int builtin_strlen(char* str) {
    if (str == NULL) return 0;
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

// Built-in integer conversion
int builtin_int(char* str) {
    if (str == NULL) return 0;
    return atoi(str);
}

// Built-in exit function
void builtin_exit(int code) {
    exit(code);
}

// Colombian hero functions
int builtin_simon_bolivar() {
    printf("Simón Bolívar: Libertador de Colombia, Venezuela, Ecuador, Perú y Bolivia\n");
    return 0;
}

int builtin_francisco_narino() {
    printf("Francisco de Paula Santander y Nariño: Héroe de la independencia colombiana\n");
    return 0;
}

int builtin_maria_cano() {
    printf("María Cano: Líder obrera y feminista colombiana\n");
    return 0;
}

int builtin_jorge_eliecer_gaitan() {
    printf("Jorge Eliécer Gaitán: Líder político y defensor del pueblo colombiano\n");
    return 0;
}

int builtin_gabriel_garcia_marquez() {
    printf("Gabriel García Márquez: Nobel de Literatura, autor de Cien Años de Soledad\n");
    return 0;
}