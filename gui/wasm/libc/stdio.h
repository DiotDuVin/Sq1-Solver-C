/* stdio.h minimal pour la compilation WebAssembly autonome (sans libc).
   printf / fprintf sont redéfinis par sq1_wasm.c avant l'inclusion du solveur. */
#ifndef SQ1_STDIO_H
#define SQ1_STDIO_H
#define stderr ((void *)0)
#endif
