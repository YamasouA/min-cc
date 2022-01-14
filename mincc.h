#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// tokenize.c
//

// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_IDENT, // 変数
  TK_NUM, // 整数トークン
  TK_EOF, // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
  TokenKind kind; // トークンの型
  Token *next; // 次の入力トークン
  long val; // kindがTK_NUMの場合、その数値
  char *str; // トークンの文字列
  int len; // トークンの長さ
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
char *strndup(char *p, int len);
Token *consume_ident();
void expect(char *op);
int expect_number();
char *expect_ident();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize();

char *user_input;

// 現在注目しているトークン
Token *token;

//
// parse.c
//

// ローカル変数
typedef struct Var Var;
struct Var {
  Var *next;
  char *name; // Variable name
  int offset; // Offset from RBP
};

typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_EQ, // ==
  ND_NE, // !=
  ND_LT, // <
  ND_LE, // <=
  ND_ASSIGN, // =
  ND_RETURN, // "return"
  ND_IF, // "if"
  ND_WHILE, // "while"
  ND_FOR, // "for"
  ND_BLOCK, // { ... }
  ND_FUNCALL, // Function call
  ND_EXPR_STMT, // Expression statement
  ND_VAR, // variable
  ND_NUM, // Integer
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node {
  NodeKind kind; // Node kind
  Node *next; // Next node

  Node *lhs; // Left-hand side
  Node *rhs; // Right-hand side

  // "if" or "while", "for" statement
  Node *cond;
  Node *then;
  Node *els;
  Node *init;
  Node *inc;

  Node *body;

  // Function call
  char *funcname;
  Node *args;

  Var *var; // Used if kind == ND_VAR
  int val; // Used if kind == ND_NUM
};

typedef struct Function Function;
struct Function {
  Function *next;
  char *name;
  Node *node;
  Var *locals;
  int stack_size;
};

Function *program();

//
// codegen.c
//

void codegen(Function *prog);
