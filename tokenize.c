#include "mincc.h"

// エラー報告用関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void verror_at(char *loc, char *fmt, va_list ap) {
  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, ""); // 空文字pos桁の幅で出力する
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Reports an error location and exit.
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(loc, fmt, ap);
}

// Reports an error location and exit.
void error_tok(Token *tok, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (tok)
    verror_at(tok->str, fmt, ap);

  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

char *strndup(char *p, int len) {
  char *buf = malloc(len + 1);
  strncpy(buf, p, len);
  buf[len] = '\0';
  return buf;
}

// トークナイズするときは長いトークンから先にトークナイズする必要がある

// カレントトークンが引数の文字列と一致するなら真を返す
Token *peek(char *s) {
  if (token->kind != TK_RESERVED || strlen(s) != token->len || memcmp(token->str, s, token->len))
    return NULL;
  return token;
}

// 与えられた文字列と一致するならカレントトークンを消費する
Token *consume(char *s) {
  if (!peek(s))
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

// 見ているトークンが変数なら消費する
Token *consume_ident() {
  if (token->kind != TK_IDENT)
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

// 次のトークンが期待している文字列の時には、トークンを一つ読み進める
// それ以外の場合にはエラーを報告する
void expect(char *s) {
  if (!peek(s))
    error_tok(token, "expected \"%s\"", s);
  token = token->next;
}

// 次のトークンが数値の場合、トークンを一つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する
int expect_number() {
  if (token->kind != TK_NUM)
    error_tok(token, "数ではありません");
  int val = token->val;
  token = token->next;
  return val;
}

// 次のトークンがTK _IDENTであるかを確認する
char *expect_ident() {
  if (token->kind != TK_IDENT)
    error_tok(token, "expected an identifier");
  char *s = strndup(token->str, token->len);
  token = token->next;
  return s;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

bool startswitch(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}

bool is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

bool is_alnum(char c) {
  return is_alpha(c) || ('0' <= c && c <= '9');
}

char *starts_with_reserved(char *p) {
  // Keyword
  static char *kw[] = {"return", "if", "else", "while", "for", "int","char", "sizeof"};

  for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
    int len = strlen(kw[i]);
    if (startswitch(p, kw[i]) && !is_alnum(p[len]))
        return kw[i];
  }

  // Multi-letter punctuator
  static char *ops[] = {"==", "!=", "<=", ">="};

  for (int i = 0; i < sizeof(ops) / sizeof(*ops); i++)
    if (startswitch(p, ops[i]))
        return ops[i];

  return NULL;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize() {
  char *p = user_input;
  Token head;
  head.next = NULL;
  Token *cur = &head; // curにトークナイズしたものを繋げていく

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    char *kw = starts_with_reserved(p);
    if (kw) {
      int len = strlen(kw);
      cur = new_token(TK_RESERVED, cur, p, len);
      p += len;
      continue;
    }
    
    if (strchr("+-*/()<>;={},&[]", *p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    if (is_alpha(*p)) {
      char *q = p++;
      // 変数名の間読み進める
      while (is_alnum(*p))
        p++;
      cur = new_token(TK_IDENT, cur, q, p - q);
      continue;
    }

    // 文字リテラル
    if (*p == '"') {
      char *q = p++;
      while (*p && *p != '"')
        p++;
      if (!*p)
        error_at(q, "unclosed string literal");
      p++;

      cur = new_token(TK_STR, cur, q, p - q); // ""も含める
      cur->contents = strndup(q + 1, p - q - 2); // ""この中身のみを取り出す
      cur->cont_len = p - q - 1;
      continue;
  }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    error_at(p, "トークンエラー");
  }
  new_token(TK_EOF, cur, p, 0);
  return head.next;
}
