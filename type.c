#include "mincc.h"

Type *int_type() {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_INT;
  return ty;
}

Type *pointer_to(Type *base) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_PTR;
  ty->base = base; // ~へのポインタ型
  return ty;
}

Type *array_of(Type *base, int size) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_ARRAY;
  ty->base = base;
  ty->array_size = size;
  return ty;
}

// tyの型情報から必要バイト数を返す
int size_of(Type *ty) {
  if (ty->kind == TY_INT || ty->kind == TY_PTR)
    return 8;
  assert(ty->kind == TY_ARRAY);
  return size_of(ty->base) * ty->array_size;
}

void visit(Node *node) {
  if (!node)
    return;

  visit(node->lhs);
  visit(node->rhs);
  visit(node->cond);
  visit(node->then);
  visit(node->els);
  visit(node->init);
  visit(node->inc);

  for (Node *n = node->body; n; n = n->next)
    visit(n);
  for (Node *n = node->args; n; n = n->next)
    visit(n);

  switch (node->kind) {
  case ND_MUL:
  case ND_DIV:
  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE:
  case ND_FUNCALL:
  case ND_NUM:
    node->ty = int_type();
    return;
  case ND_VAR:
    node->ty = node->var->ty; // パースするときにpush_varで変数の型を取得してある
    return;
  case ND_ADD:
    if (node->rhs->ty->base) { // ポインタの演算するときはポインタが左、右はポインタ以外 (e.g. &x+1)
      Node *tmp = node->lhs;
      node->lhs = node->rhs;
      node->rhs = tmp;
    }
    if (node->rhs->ty->base) // 右と左の枝が両方ともポインタならエラー
      error_tok(node->tok, "invalid pointer arithmetic operands");
    node->ty = node->lhs->ty; // 左の子の型が親の型
    return;
  case ND_SUB:
    if (node->rhs->ty->kind == TY_PTR) // 上と同じ
      error_tok(node->tok, "invalid pointer arithmetic operands");
    node->ty = node->lhs->ty;
    return;
  case ND_ASSIGN:
    node->ty = node->lhs->ty; // 等式の型は左辺値の値を使う
    return;
  case ND_ADDR:
    // node->ty = pointer_to(node->lhs->ty);
    if (node->lhs->ty->kind == TY_ARRAY)
        node->ty = pointer_to(node->lhs->ty->base);
    else
        node->ty = pointer_to(node->lhs->ty);
    return;
  case ND_DEREF:
    if (!node->lhs->ty->base)
      error_tok(node->tok, "invalid pointer dereference");
    node->ty = node->lhs->ty->base;
    return;
  }
}

void add_type(Function *prog) {
  for (Function *fn = prog; fn; fn = fn->next)
    for (Node *node = fn->node; node; node = node->next)
      visit(node);
}

