
#include "../all.h"

NsCompiler* init_nsc() {
  NsCompiler* nsc = malloc(sizeof(NsCompiler));
  nsc->name = NULL;
  nsc->pkc = NULL;
  nsc->scope = init_scope();
  return nsc;
}

void free_nsc(NsCompiler* nsc) {
  //
  free(nsc);
}
