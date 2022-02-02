#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type Type;
typedef struct Member Member;

//
// tokenize.c
//

// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_IDENT, // 変数
  TK_STR, // 文字リテラル
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

  char *contents; // 文字リテラルの内容'\0'を含む
  char cont_len; // 文字リテラルの長さ
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
Token *peek(char *s);
Token *consume(char *op);
char *strndup(char *p, int len);
Token *consume_ident();
void expect(char *op);
int expect_number();
char *expect_ident();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize();

extern char *filename;
extern char *user_input;

// 現在注目しているトークン
extern Token *token;

//
// parse.c
//

// ローカル変数
typedef struct Var Var;
struct Var {
  char *name; // Variable name
  Type *ty; // Type
  bool is_local; // local or global

  // Local variable
  int offset; // Offset from RBP

  // Global variable
  char *contents;
  int cont_len;
};

typedef struct VarList VarList;
struct VarList {
  VarList *next;
  Var *var;
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
  ND_MEMBER, // . (struct member access)
  ND_ADDR, // unary &
  ND_DEREF, // unary *
  ND_RETURN, // "return"
  ND_IF, // "if"
  ND_WHILE, // "while"
  ND_FOR, // "for"
  ND_SIZEOF, // "sizeof"
  ND_BLOCK, // { ... }
  ND_FUNCALL, // Function call
  ND_EXPR_STMT, // Expression statement
  ND_STMT_EXPR, // Statement expression
  ND_VAR, // variable
  ND_NUM, // Integer
  ND_NULL, // Empty statement
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node {
  NodeKind kind; // Node kind
  Node *next; // Next node
  Type *ty; // Type, e.g. int or pointer to int
  Token *tok; // Representative token

  Node *lhs; // Left-hand side
  Node *rhs; // Right-hand side

  // "if" or "while", "for" statement
  Node *cond;
  Node *then;
  Node *els;
  Node *init;
  Node *inc;

  // Block or statement expression
  Node *body;

  // Struct member access
  char *member_name;
  Member *member;

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
  VarList *params;
  Node *node;
  VarList *locals;
  int stack_size;
};

typedef struct {
  VarList *globals;
  Function *fns;
} Program;
Program *program();

//
// type.c
//

typedef enum {
  TY_VOID,
  TY_CHAR,
  TY_SHORT,
  TY_INT,
  TY_LONG,
  TY_PTR,
  TY_ARRAY,
  TY_STRUCT,
  TY_FUNC,
} TypeKind;

struct Type {
  TypeKind kind;
  int align; // alignment
  Type *base; // pointer or array
  int array_size; // array
  Member *members; // struct
  Type *return_ty; // function
};

// Struct member
struct Member {
  Member *next;
  Type *ty;
  char *name;
  int offset;
};

int align_to(int n, int align);
Type *void_type();
Type *char_type();
Type *short_type();
Type *int_type();
Type *long_type();
Type *func_type(Type *return_ty);
Type *pointer_to(Type *base);
Type *array_of(Type *base, int size);
int size_of(Type *ty);

void add_type(Program *prog);

//
// codegen.c
//

void codegen(Program *prog);
