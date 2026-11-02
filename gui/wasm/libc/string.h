/* string.h minimal : les fonctions sont implémentées dans sq1_wasm.c. */
#ifndef SQ1_STRING_H
#define SQ1_STRING_H
typedef __SIZE_TYPE__ size_t;
void  *memcpy(void *dst, const void *src, size_t n);
void  *memmove(void *dst, const void *src, size_t n);
void  *memset(void *dst, int c, size_t n);
size_t strlen(const char *s);
char  *strncpy(char *dst, const char *src, size_t n);
int    strncmp(const char *a, const char *b, size_t n);
#endif
