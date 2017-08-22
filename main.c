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

//// General parser functions

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

//// Custom printer

void print_ast_node(mpc_ast_t* t, int depth) {
  for (int i = 0; i < depth; ++i)
    putchar(' ');

  printf("tag= %s, con=%s, s=(%ld,%ld,%ld), n=%d\n",
         t->tag,
         t->contents,
         t->state.pos, t->state.row, t->state.col,
         t->children_num);
}

void parse_handler_print(mpc_ast_t* t) {
  traverse_ast(t, &print_ast_node, 0);
}

//// Eval 

enum { LVAL_NUM, LVAL_ERR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

typedef struct {
  int type;
  long num;
  int err;
} lval;

lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

void lval_print(lval v) {
  switch (v.type) {
    /* In the case the type is a number print it */
    /* Then 'break' out of the switch. */
  case LVAL_NUM: printf("%li", v.num); break;

    /* In the case the type is an error */
  case LVAL_ERR:
    /* Check what type of error it is and print it */
    if (v.err == LERR_DIV_ZERO) {
      printf("Error: Division By Zero!");
    }
    if (v.err == LERR_BAD_OP)   {
      printf("Error: Invalid Operator!");
    }
    if (v.err == LERR_BAD_NUM)  {
      printf("Error: Invalid Number!");
    }
    break;
  }
}

/* Print an "lval" followed by a newline */
void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval eval_op(lval x, char* op, lval y) {
  /* If either value is an error return it */
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  /* Otherwise do maths on the number values */
  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) {
    /* If second operand is zero return error */
    return y.num == 0
      ? lval_err(LERR_DIV_ZERO)
      : lval_num(x.num / y.num);
  }

  return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) {
    /* Check if there is some error in conversion */
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  char* op = t->children[1]->contents;
  lval x = eval(t->children[2]);

  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

void parse_handler_eval(mpc_ast_t* t) {
  lval result = eval(t);
  lval_println(result);
}

int main(int argc, char** argv) {
  puts("build-your-own-lisp Version 0.0.1");
  puts("Press Ctrl-C to Exit\n");

  create_parser();

  while (1) {
    char* input = readline("lispy> ");
    add_history(input);

    /* parse(input, &parse_handler_print); */
    parse(input, &parse_handler_eval);
    /* parse(input, &mpc_print); */

    /* printf("\n===========\n"); */

    free(input);
  }

  cleanup_parser();

  return 0;
}
