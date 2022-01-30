#include "mincc.h"

Type *new_type(TypeKind kind) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = kind;
  return ty;
}

Type *char_type() {
  return new_type(TY_CHAR);
}

Type *int_type() {
  return new_type(TY_INT);
}

Type *pointer_to(Type *base) {
  Type *ty = new_type(TY_PTR);
  ty->base = base; // ~へのポインタ型
  return ty;
}

Type *array_of(Type *base, int size) {
  Type *ty = new_type(TY_ARRAY);
  ty->base = base;
  ty->array_size = size;
  return ty;
}

// tyの型情報から必要バイト数を返す
int size_of(Type *ty) {
  switch (ty->kind) {
  case TY_CHAR:
    return 1;
  case TY_INT:
  case TY_PTR:
    return 8;
  case TY_ARRAY:
    return size_of(ty->base) * ty->array_size;
  default:
    assert(ty->kind == TY_STRUCT);
    Member *mem = ty->members;
    while (mem->next)
      mem = mem->next;
    return mem->offset + size_of(mem->ty); // 最後のメンバのオフセット + 最後のメンバのサイズ
  }
}

Member *find_member(Type *ty, char *name) {
  assert(ty->kind == TY_STRUCT);
  for (Member *mem = ty->members; mem; mem = mem->next)
    if (!strcmp(mem->name, name))
      return mem;
  return NULL;
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
  case ND_MEMBER: {
    if (node->lhs->ty->kind != TY_STRUCT)
      error_tok(node->tok, "not a struct");
    node->member = find_member(node->lhs->ty, node->member_name);
    if (!node->member)
      error_tok(node->tok, "specified member does not exist");
    node->ty = node->member->ty;
    return;
  }
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
  case ND_SIZEOF:
    node->kind = ND_NUM;
    node->ty = int_type();
    node->val = size_of(node->lhs->ty);
    node->lhs = NULL;
    return;
  // Statements and Declarations in Expressions
  case ND_STMT_EXPR: {
    Node *last = node->body;
    while (last->next)
      last = last->next;
    node->ty = last->ty;
    return;
  }
  }
}

void add_type(Program *prog) {
  for (Function *fn = prog->fns; fn; fn = fn->next)
    for (Node *node = fn->node; node; node = node->next)
      visit(node);
}

