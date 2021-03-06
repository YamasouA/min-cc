#include "mincc.h"


// Returns the contents of a given file.
char *read_file(char *path) {
  // Open and read the file
  FILE*fp = fopen(path, "r");
  if (!fp)
    error("cannot open %s: %s", path, strerror(errno));

  int filemax = 10 * 1024 * 1024;
  char *buf = malloc(filemax);
  
  // size 1だけの大きさの最大filemax-2(\n\0を後から追加するから-2)個のオブジェクトがfpから配列bufに読み込まれる
  // freadは読み込まれたオブジェクトの数を返す
  int size = fread(buf, 1, filemax - 2, fp); 
 
  // feof(fp)はファイルの終わりに達したかを調べる
  if (!feof(fp))
    error("%s: file too large");

  // Make sure that the string ends with "\n\0".
  if (size == 0 || buf[size - 1] != '\n')
    // コンパイラの都合上全ての行が改行文字で終わっている方が、改行文字かEOFで終わっているデータよりも扱いやすいので\nを追加する
    buf[size++] = '\n';
  buf[size] = '\0';
  return buf;
}


int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  // tokenize and parse
  filename = argv[1];
  user_input = read_file(argv[1]);
  token = tokenize();
  Program *prog = program();
  add_type(prog);

  // assign offsets to local variables
  for (Function *fn = prog->fns; fn; fn = fn->next) {
    int offset = 0;
    for (VarList *vl = fn->locals; vl; vl = vl->next) {
      Var *var = vl->var;
      offset = align_to(offset, var->ty->align);
      offset += size_of(var->ty, var->tok);
      var->offset = offset;
    }
      fn->stack_size = align_to(offset, 8);
  }

  codegen(prog);
  return 0;
}
