#include "mincc.h"

int labelseq = 0;

// 事前に確保していた変数の領域の対応するアドレスに値を入れる
void gen_addr(Node *node) {
  if (node->kind == ND_VAR) {
    printf("  lea rax, [rbp-%d]\n", node->var->offset); // leaは[src]のアドレス計算を行う
    printf("  push rax\n");
    return;
  }

  error("not an lvalue");
}

void load() {
  printf("  pop rax\n");
  printf("  mov rax, [rax]\n"); // raxに入っているアドレスの値を[rax]で参照
  printf("  push rax\n");
}

// =がある時にraxにrdiを代入する
void store() {
  printf("  pop rdi\n");
  printf("  pop rax\n");
  printf("  mov [rax], rdi\n");
  printf("  push rdi\n");
}

// 与えられたノードからコードを作成する
void gen(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    printf("  push %ld\n", node->val);
    return;
  case ND_EXPR_STMT:
    gen(node->lhs);
    printf("  add rsp, 8\n"); // returnしないから計算した値を破棄してる？
    return;
  case ND_VAR:
    gen_addr(node); // 変数のアドレスをrspに格納
    load(); // 変数の値を取り出して変数をrspに入れる
    return;
  case ND_ASSIGN:
    gen_addr(node->lhs); // 左辺値のアドレスをrspに格納される
    gen(node->rhs); // 右辺値の結果がrspに格納される
    store(); // 左辺に右辺を代入する
    return;
  case ND_IF: {
    int seq = labelseq++;
    if (node->els) {
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je .Lelse%d\n", seq); // .Lをつけると別ファイルのラベルと衝突しない
      gen(node->then);
      printf("  jmp .Lend%d\n", seq);
      printf(".Lelse%d:\n", seq);
      gen(node->els);
      printf(".Lend%d:\n", seq);
    } else {
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .Lend%d\n", seq);
      gen(node->then);
      printf(".Lend%d:\n", seq);
    }
    return;
  }
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  jmp .Lreturn\n");
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
    case ND_ADD:
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      printf("  sub rax, rdi\n");
      break;
    case ND_MUL:
      printf("  imul rax, rdi\n");
      break;
    case ND_DIV:
      printf("  cqo\n"); // 64bitを128bitにする
      printf("  idiv rdi\n");
      break;
    case ND_EQ:
      printf("  cmp rax, rdi\n");
      printf("  sete al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_NE:
      printf("  cmp rax, rdi\n");
      printf("  setne al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_LT:
      printf("  cmp rax, rdi\n");
      printf("  setl al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_LE:
      printf("  cmp rax, rdi\n");
      printf("  setle al\n");
      printf("  movzb rax, al\n");
      break;
  }

  printf("  push rax\n");
}

void codegen(Program *prog) {
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    printf("  push rbp\n");
    printf("  mov rbp, rsp\n"); // 呼び出し時点のアドレスをrbpに入れる
    printf("  sub rsp, %d\n", prog->stack_size); // 変数26個分の領域を確保する

    for (Node *node = prog->node; node; node =node->next)
      gen(node);

    // 最後の式の結果がRAXに残っているのでそれが返り値
    printf("  .Lreturn:\n");
    printf("  mov rsp, rbp\n"); // 呼び出しもとのアドレスに変える
    printf("  pop rbp\n");
    printf("  ret \n");
}
