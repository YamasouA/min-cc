#include "mincc.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  // tokenize and parse
  user_input = argv[1];
  token = tokenize();
  Node *node = expr();

  codegen(node);
  return 0;
}
