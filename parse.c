#include "mincc.h"

Var *locals;

// Find a local variable by name
Var *find_var(Token *tok) {
  for (Var *var = locals; var; var = var->next)
    if (strlen(var->name) == tok->len && !memcmp(tok->str, var->name, tok->len))
      return var;
  return NULL;
}

Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_unary(NodeKind kind, Node *expr) {
  Node *node = new_node(kind);
  node->lhs = expr;
  return node;
}

Node *new_var(Var *var) {
  Node *node = new_node(ND_VAR);
  node->var = var;
  return node;
}

// ローカル変数を連結リストに繋げる
Var *push_var(char *name) {
  Var *var = calloc(1, sizeof(Var));
  var->next = locals;
  var->name = name;
  locals = var;
  return var;
}

Node *new_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

Node *stmt(void);
Node *expr(void); 
Node *assign(void);
Node *equality(void);
Node * relational(void);
Node *add(void);
Node *mul(void);
Node *unary(void);
Node *primary(void);

// program = stmt*
Program *program() {
  locals = NULL;

  Node head;
  head.next = NULL;
  Node *cur = &head;

  while (!at_eof()) {
    cur->next = stmt();
    cur = cur->next;
  }

  Program *prog = calloc(1, sizeof(Program));
  prog->node = head.next;
  prog->locals = locals;
  return prog;
}

Node *read_expr_stmt() {
  return new_unary(ND_EXPR_STMT, expr());
}

// stmt = "return" expr ";" 
//        | "if" "(" expr ")" stmt ("else" stmt)?
//        | "while" "(" expr ")" stmt
//        | expr ";"
Node *stmt() {
  if (consume("return")) {
    Node *node = new_unary(ND_RETURN, expr());
    expect(";");
    return node;
  }

  if (consume("if")) {
    Node *node = new_node(ND_IF);
    expect("(");
    node->cond = expr(); // 条件
    expect(")");
    node->then = stmt(); // 条件が真の時のプログラム
    if (consume("else"))
      node->els = stmt();
    return node;
  }

  if (consume("while")) {
    Node *node = new_node(ND_WHILE);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    return node;
  }

  Node *node = read_expr_stmt();
  expect(";");
  return node;
}


// expr = assign
Node *expr(void) {
  return assign();
}

// assign = equality ("=" assign)?
Node *assign() {
  Node *node = equality();
  if (consume("="))
    node = new_binary(ND_ASSIGN, node, assign());
  return node;
}
// equality = relational ("==" relational | "!=" relational)*
Node *equality(void) {
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_binary(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_binary(ND_NE, node, relational());
    else
      return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | "<" add)*
Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<"))
      node = new_binary(ND_LT, node, add());
    else if (consume("<="))
      node = new_binary(ND_LE, node, add());
    else if (consume(">"))
      node = new_binary(ND_LT, add(), node);
    else if (consume(">="))
      node = new_binary(ND_LE, add(), node);
    else
      return node;
  }
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_binary(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_binary(ND_SUB, node, mul());
    else
      return node;
  }
}

// mul = primary ("*" primary | "/" primary)*
Node *mul(void) {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_binary(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_binary(ND_DIV, node, unary());
    else
      return node;
  }
}

// unary = ("+" | "-")? primary
Node *unary(void) {
  if (consume("+"))
    return primary();
  if (consume("-"))
    return new_binary(ND_SUB, new_num(0), unary());
  return primary();
}

// primary = "(" expr ")" | ident | num
Node *primary(void) {
  if (consume("(")) {
    Node *node = expr(); // ()の中の指揮を木に変換する
    expect(")"); // ')'を飛ばす
    return node;
  }
  Token *tok = consume_ident(); // ここまで来たらnumかidentしか残ってない
  if (tok) {
    Var *var = find_var(tok);
    if (!var)
      var = push_var(strndup(tok->str, tok->len));
    return new_var(var);
  }
  return new_num(expect_number());
}
