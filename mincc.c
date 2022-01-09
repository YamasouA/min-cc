#include "mincc.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  // tokenize and parse
  user_input = argv[1];
  token = tokenize();
  Program *prog = program();

  // assign offsets to local variables
  int offset = 0;
  for (Var *var = prog->locals; var; var = var->next) {
    offset += 8;
    var->offset = offset;
  }
  prog->stack_size = offset;

  codegen(prog);
  return 0;
}
