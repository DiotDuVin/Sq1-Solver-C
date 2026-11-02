/* stdlib.h minimal (atoi n'est utilisé que par main(), exclu du build WASM). */
#ifndef SQ1_STDLIB_H
#define SQ1_STDLIB_H
#define NULL ((void *)0)
int atoi(const char *s);
#endif
