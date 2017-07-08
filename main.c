#include <stdio.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>
#include "mpc.h"

// Build command:
// cc -std=c99 -Wall main.c mpc.c -lreadline -o repl

static mpc_parser_t* Number;
static mpc_parser_t* Operator;
static mpc_parser_t* Expr;
static mpc_parser_t* Lispy;

// General parser functions

void create_parser() {
  // polish notation grammar
  Number   = mpc_new("number");
  Operator = mpc_new("operator");
  Expr     = mpc_new("expr");
  Lispy    = mpc_new("lispy");

  mpca_lang(
    MPCA_LANG_DEFAULT,
    "                                                   \
    number   : /-?[0-9]+/ ;                             \
    operator : '+' | '-' | '*' | '/' ;                  \
    expr     : <number> | '(' <operator> <expr>+ ')' ;  \
    lispy    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Operator, Expr, Lispy);
}

void cleanup_parser() {
  mpc_cleanup(4, Number, Operator, Expr, Lispy);
}

void parse(char* line, void (*parse_fnc)(mpc_ast_t*)) {
  mpc_result_t r;
  if (mpc_parse("<stdin>", line, Lispy, &r)) {
    parse_fnc(r.output);
    mpc_ast_delete(r.output);
  } else {
    mpc_err_print(r.error);
    mpc_err_delete(r.error);
  }
}

typedef void (*visit_node_fnc_t) (mpc_ast_t*, int);
void traverse_ast(mpc_ast_t* t, visit_node_fnc_t f, int depth) {
  if (!t || !f) return;

  f(t, depth);

  for (int i = 0; i < t->children_num; ++i)
    traverse_ast(t->children[i], f, depth + 1);
}

// Custom printer

void print_ast_node(mpc_ast_t* t, int depth) {
  for (int i = 0; i < depth; ++i)
    putchar(' ');

  printf("tag= %s, con=%s, s=(%ld,%ld,%ld), n=%d\n",
         t->tag,
         t->contents,
         t->state.pos, t->state.row, t->state.col,
         t->children_num);
}

long sub_eval(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  char* op = t->children[1]->contents;
  long result = sub_eval(t->children[2]);
  for (int i = 3; i < t->children_num - 1; ++i) {
    long value = sub_eval(t->children[i]);

    switch (*op) {
    case '+': result += value; break;
    case '-': result -= value; break;
    case '*': result *= value; break;
    case '/': result /= value; break;
    }
  }

  return result;
}

void custom_print(mpc_ast_t* t) {
  traverse_ast(t, &print_ast_node, 0);
}


void custom_eval(mpc_ast_t* t) {
  printf(">> %ld\n", sub_eval(t));
}


int main(int argc, char** argv) {
  puts("build-your-own-lisp Version 0.0.1");
  puts("Press Ctrl-C to Exit\n");

  create_parser();

  while (1) {
    char* input = readline("lispy> ");
    add_history(input);

    /* parse(input, &custom_print); */
    parse(input, &custom_eval);
    /* parse(input, &mpc_print); */
    /* printf("\n===========\n"); */

    free(input);
  }

  cleanup_parser();

  return 0;
}
