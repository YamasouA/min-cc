#include "mincc.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  // tokenize and parse
  user_input = argv[1];
  token = tokenize();
  Function *prog = program();
  add_type(prog);

  // assign offsets to local variables
  for (Function *fn = prog; fn; fn = fn->next) {
    int offset = 0;
    for (VarList *vl = fn->locals; vl; vl = vl->next) {
      Var *var = vl->var;
      offset += size_of(var->ty);
      var->offset = offset;
    }
      fn->stack_size = offset;
  }

  codegen(prog);
  return 0;
}
