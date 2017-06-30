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

void lispy_create()
{
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

void lispy_cleanup()
{
  mpc_cleanup(4, Number, Operator, Expr, Lispy);
}

void lispy_parse(char* line)
{
  mpc_result_t r;
  if (mpc_parse("<stdin>", line, Lispy, &r)) {
    mpc_ast_print(r.output);
    mpc_ast_delete(r.output);
  } else {
    mpc_err_print(r.error);
    mpc_err_delete(r.error);
  }
}


int main(int argc, char** argv) {

  puts("build-your-own-lisp Version 0.0.1");
  puts("Press Ctrl-C to Exit\n");

  lispy_create();

  while (1) {
    char* input = readline("lispy> ");
    add_history(input);

    lispy_parse(input);
    // printf(">> %s\n", input);

    free(input);
  }

  lispy_cleanup();

  return 0;
}
