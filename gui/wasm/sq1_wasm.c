/*
 * Interface WebAssembly du solveur Square-1.
 *
 * Ce fichier inclut directement ../../sq1_solver.c (sans son main) et expose
 * quelques fonctions appelées depuis le JavaScript de l'interface graphique.
 * Il se compile sans libc (clang --target=wasm32 -ffreestanding -nostdlib),
 * voir gui/build.sh.
 */

#define SQ1_NO_MAIN
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- mini libc ---------------------------------------------------------- */

void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = dst; const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dst;
}
void *memmove(void *dst, const void *src, size_t n) {
    unsigned char *d = dst; const unsigned char *s = src;
    if (d < s) while (n--) *d++ = *s++;
    else { d += n; s += n; while (n--) *--d = *--s; }
    return dst;
}
void *memset(void *dst, int c, size_t n) {
    unsigned char *d = dst;
    while (n--) *d++ = (unsigned char)c;
    return dst;
}
size_t strlen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }
char *strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; i++) dst[i] = src[i];
    for (; i < n; i++) dst[i] = 0;
    return dst;
}
int strncmp(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
        if (!a[i]) return 0;
    }
    return 0;
}

/* ---- redirection des sorties du solveur --------------------------------- */

static char errbuf[160];

/* Copie le message d'erreur (seul %c est utilisé par ReadPosition). */
static void sq1_seterr(const char *fmt, ...) {
    va_list ap; int n = 0;
    va_start(ap, fmt);
    for (; *fmt && n < (int)sizeof(errbuf) - 2; fmt++) {
        if (fmt[0] == '%' && fmt[1] == 'c') { errbuf[n++] = (char)va_arg(ap, int); fmt++; }
        else if (*fmt != '\n') errbuf[n++] = *fmt;
    }
    errbuf[n] = 0;
    va_end(ap);
}

#define printf(...) ((void)0)
#define fprintf(stream, ...) sq1_seterr(__VA_ARGS__)

#include "../../sq1_solver.c"

#undef printf
#undef fprintf

/* ---- tampons partagés avec JavaScript ----------------------------------- */

#define EXPORT(name) __attribute__((export_name(#name)))

static char inbuf[64];
static char outbuf[32768];
static int outlen;

static void out_s(const char *s) {
    while (*s && outlen < (int)sizeof(outbuf) - 1) outbuf[outlen++] = *s++;
    outbuf[outlen] = 0;
}
static void out_c(char c) { char s[2] = { c, 0 }; out_s(s); }
static void out_i(long v) {
    char s[24]; int n = 0;
    if (v < 0) { out_c('-'); v = -v; }
    do { s[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) out_c(s[--n]);
}
static void out_turn(int a) {           /* même convention que print_turn */
    if (a <= 6) out_i(a); else out_i(a - 12);
}
static char ml_char(char ml) { return ml > 0 ? '-' : ml < 0 ? '/' : '?'; }

EXPORT(sq1_inbuf)  char *sq1_inbuf(void)  { return inbuf; }
EXPORT(sq1_outbuf) char *sq1_outbuf(void) { return outbuf; }

static int ready = 0;
EXPORT(sq1_init) void sq1_init(void) {
    if (ready) return;
    InitLayers();
    InitShapeTable();
    InitPermTable();
    ready = 1;
}

/*
 * Résout la position (format utilisateur, 16/17 caractères) écrite dans inbuf.
 * Écrit un objet JSON dans outbuf et renvoie sa longueur :
 *   {"ok":true,"twists":n,"phase1":k,"nodes1":..,"nodes2":..,
 *    "start":"<24 cases + couche médiane>",
 *    "steps":[{"label":"(1,-2)","top":1,"bottom":10,"twist":false,"state":"..."}, ...]}
 * ou {"ok":false,"error":"..."}.
 */
EXPORT(sq1_solve) int sq1_solve(void) {
    Pos1 p, q;
    int i, t, b;

    outlen = 0; outbuf[0] = 0; errbuf[0] = 0;
    sq1_init();

    if (ReadPosition(inbuf, &p)) {
        out_s("{\"ok\":false,\"error\":\"");
        out_s(errbuf[0] ? errbuf : "position invalide");
        out_s("\"}");
        return outlen;
    }

    solutionLen = 0;
    if (SolveOne(&p, 0) < 0) {
        out_s("{\"ok\":false,\"error\":\"Aucune solution trouvée (vérifiez la position).\"}");
        return outlen;
    }

    for (i = t = 0; i < solutionLen; i++) if (solutionMoves[i] == 0) t++;
    out_s("{\"ok\":true,\"twists\":"); out_i(t);
    out_s(",\"phase1\":"); out_i(Length1);
    out_s(",\"nodes1\":"); out_i(Nodes1);
    out_s(",\"nodes2\":"); out_i(Nodes2);
    out_s(",\"start\":\"");
    p.pos[24] = 0; out_s(p.pos); out_c(ml_char(p.ml));
    out_s("\",\"steps\":[");

    /* Rejoue la solution pour fournir l'état après chaque étape */
    q = p;
    for (i = 0; i < solutionLen; i++) {
        int m = solutionMoves[i];
        if (i) out_c(',');
        if (m == 0) {
            TwistLayers(&q);
            q.ml = (char)-q.ml;
            out_s("{\"label\":\"/\",\"top\":0,\"bottom\":0,\"twist\":true");
        } else {
            t = 0; b = 0;
            if (m > 0) {
                t = m;
                if (i + 1 < solutionLen && solutionMoves[i + 1] < 0) b = -solutionMoves[++i];
            } else {
                b = -m;
            }
            if (t) TurnLayer(q.pos, t);
            if (b) TurnLayer(q.pos + 12, b);
            out_s("{\"label\":\"("); out_turn(t); out_c(','); out_turn(b);
            out_s(")\",\"top\":"); out_i(t);
            out_s(",\"bottom\":"); out_i(b);
            out_s(",\"twist\":false");
        }
        out_s(",\"state\":\""); q.pos[24] = 0; out_s(q.pos); out_c(ml_char(q.ml));
        out_s("\"}");
    }
    out_s("]}");
    return outlen;
}
