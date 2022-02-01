#include "mincc.h"

int labelseq = 0;
char *funcname;
char *argreg1[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
char *argreg2[] = {"di", "si", "dx", "cx", "r8w", "r9w"};
char *argreg4[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
char *argreg8[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

void gen(Node *node);

// 事前に確保していた変数の領域の対応するアドレスに値を入れる
void gen_addr(Node *node) {
  switch (node->kind) {
  case ND_VAR: {
    Var *var = node->var;
    if (var->is_local) {
      printf("  lea rax, [rbp-%d]\n", node->var->offset); // leaはraxに[rbp-%d]のアドレスを入れる
      printf("  push rax\n");
    } else {
      printf("  push offset %s\n", var->name);
    }
    return;
  }
  case ND_DEREF:
    gen(node->lhs);
    return;
  case ND_MEMBER:
    gen_addr(node->lhs);
    printf("  pop rax\n");
    printf("  add rax, %d\n", node->member->offset);
    printf("  push rax\n");
    return;
  }

  error_tok(node->tok, "not an lvalue");
}

void gen_lval(Node *node) {
  if (node->ty->kind == TY_ARRAY)
    error_tok(node->tok, "not an lvalue");
  gen_addr(node);
}

void load(Type *ty) {
  printf("  pop rax\n");

  int sz = size_of(ty);
  if (sz == 1) {
    printf("  movsx rax, byte ptr[rax]\n"); // movsx 1byteにキャスト, movsx ecx, BYTE PTR [rax]とすると、RAXが指しているアドレスから1バイト読み込んでECXに入れることができる
  } else if (sz == 2) {
    printf("  movsx rax, word ptr [rax]\n");
  } else if (sz == 4) {
    printf("  movsxd rax, dword ptr [rax]\n"); // movsxd 4byteにキャスト, dword ptrは4byte
  } else {
    printf("  mov rax, [rax]\n"); // raxに入っているアドレスから値をロードしてraxにセットする
  }

  printf("  push rax\n");
}

// =がある時にraxにrdiを代入する
void store(Type *ty) {
  printf("  pop rdi\n");
  printf("  pop rax\n");
  int sz = size_of(ty);
  if (sz == 1) {
    printf("  mov [rax], dil\n");
  } else if (sz == 2) {
    printf("  mov [rax], di\n");
  } else if (sz == 4) {
    printf("  mov [rax], edi\n");
  } else {
    assert(sz == 8);
    printf("  mov [rax], rdi\n"); // mov [rdi], raxならばRAXの値を、RDIに入っているアドレスにストアすることになる
  }

  printf("  push rdi\n");
}

// 与えられたノードからコードを作成する
void gen(Node *node) {
  switch (node->kind) {
  case ND_NULL:
    return;
  case ND_NUM:
    printf("  push %d\n", node->val);
    return;
  case ND_EXPR_STMT:
    gen(node->lhs);
    printf("  add rsp, 8\n"); // returnしないから計算した値を破棄してる？
    return;
  case ND_VAR:
  case ND_MEMBER:
    gen_addr(node); // 変数のアドレスをrspに格納
    if (node->ty->kind != TY_ARRAY) // 配列には代入
      load(node->ty); // 変数の値を取り出して変数をrspに入れる
    return;
  case ND_ASSIGN:
    gen_lval(node->lhs); // 左辺値のアドレスをrspに格納される
    gen(node->rhs); // 右辺値の結果がrspに格納される
    store(node->ty); // 左辺に右辺を代入する
    return;
  case ND_ADDR:
    gen_addr(node->lhs);
    return;
  case ND_DEREF:
    gen(node->lhs);
    if (node->ty->kind != TY_ARRAY)
      load(node->ty);
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
  case ND_WHILE: {
    int seq = labelseq++;
    printf(".Lbegin%d:\n", seq);
    gen(node->cond);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .Lend%d\n", seq);
    gen(node->then);
    printf("  jmp .Lbegin%d\n", seq);
    printf(".Lend%d:\n", seq);
    return;
  }
  case ND_FOR: {
    int seq = labelseq++;
    if (node->init)
      gen(node->init);
    printf(".Lbegin%d:\n", seq);
    if (node->cond) {
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .Lend%d\n", seq);
    }
    gen(node->then);
    if (node->inc)
      gen(node->inc);
    printf("  jmp .Lbegin%d\n", seq);
    printf(".Lend%d:\n", seq);
    return;
  }
  case ND_BLOCK:
  case ND_STMT_EXPR:
    for (Node *n = node->body; n; n = n->next)
      gen(n);
    return;
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  jmp .Lreturn.%s\n", funcname);
    return;
  case ND_FUNCALL: {
    int nargs = 0;
    // 引数分だけ、スタックに積む
    for (Node *arg = node->args; arg; arg=arg->next) {
      gen(arg);
      nargs++;
    }
    
    // スタックから取り出して、レジスタにセットする
    for (int i = nargs - 1; i >= 0; i--)
        printf("  pop %s\n", argreg8[i]);

    //We need to align RSP to a 16 byte boundary before
    // calling a function because it is an ABI requirement.
    // RAX is set to 0 for variadic function.
    int seq = labelseq++;
    printf("  mov rax, rsp\n");
    printf("  and rax, 15\n"); // raxと1111のAND命令（16の倍数だと0となる）
    printf("  jnz .Lcall%d\n", seq); // ZFフラグ(演算結果の全ビットが0の時1、それ以外の時0)がセットされていない場合に、指定された場所にジャンプする
    printf("  mov rax, 0\n"); // raxの値をリセットする
    printf("  call %s\n", node->funcname);
    printf("  jmp .Lend%d\n", seq);
    printf(".Lcall%d:\n", seq); // rspが16の倍数じゃない時
    printf("  sub rsp, 8\n"); // push, popは8の倍数で操作するから、16じゃなきゃ8の倍数になってる
    printf("  mov rax, 0\n"); // raxの値をリセット
    printf("  call %s\n", node->funcname);
    printf("  add rsp, 8\n");
    printf(".Lend%d:\n", seq);
    printf("  push rax\n");
    return;
  }
  }
  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
    case ND_ADD:
      if (node->ty->base)
        printf("  imul rdi, %d\n", size_of(node->ty->base)); // pointerや配列なら&x+1の1を適当な型サイズで進めるようにする
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      if (node->ty->base)
        printf("  imul rdi, %d\n", size_of(node->ty->base));
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

void emit_data(Program *prog) {
  printf(".data\n");

  for (VarList *vl = prog->globals; vl; vl = vl->next) {
    Var *var = vl->var;
    printf("%s:\n", var->name);

    if (!var->contents) {
      printf("  .zero %d\n", size_of(var->ty));
      continue;
    }

    for (int i = 0; i < var->cont_len; i++)
      printf("  .byte %d\n", var->contents[i]);
  }
}

void load_arg(Var *var, int idx) {
  int sz = size_of(var->ty);
  if (sz == 1) {
    printf("  mov [rbp-%d], %s\n", var->offset, argreg1[idx]);
  } else if (sz == 2) {
    printf("  mov [rbp-%d], %s\n", var->offset, argreg2[idx]);
  } else if (sz == 4) {
    printf("  mov [rbp-%d], %s\n", var->offset, argreg4[idx]);
  } else {
    assert(sz == 8);
    printf("  mov [rbp-%d], %s\n", var->offset, argreg8[idx]);
  }
}

void emit_text(Program *prog) {
  printf(".text\n");

  for (Function *fn = prog->fns; fn; fn = fn->next) {
    printf(".global %s\n", fn->name);
    printf("%s:\n", fn->name);
    funcname = fn->name;

    // Prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n"); // 呼び出し時点のアドレスをrbpに入れる
    printf("  sub rsp, %d\n", fn->stack_size); // 変数分領域確保

    // スタックに引数を積む
    int i = 0;
    for (VarList *vl = fn->params; vl; vl = vl->next) {
      load_arg(vl->var, i++);
    }

    // Emit code
    for (Node *node = fn->node; node; node = node->next)
      gen(node);
      
    // Epilogue
    printf(".Lreturn.%s:\n", funcname);
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
  }
}

void codegen(Program *prog) {
  printf(".intel_syntax noprefix\n");
  emit_data(prog);
  emit_text(prog);
}
