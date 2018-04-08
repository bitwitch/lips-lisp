#include <stdio.h> 
#include <stdlib.h>
#include <editline/readline.h>
#include "mpc.h"

typedef struct {
  int type;
  long num;
  int err;
} lval;

enum { LVAL_NUM, LVAL_ERR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

static char pct = '%'; 

lval lval_num (long x) 
{
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

lval lval_err (int x) 
{
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}


int number_of_nodes(mpc_ast_t* t) 
{
    if (t->children_num == 0) return 1; 

    // iterate through all the children and increment total for each child
    int total = 1; 
    for (int child = 0; child < t->children_num; child++) 
    {
        total += number_of_nodes(t->children[child]);
    }

    return total; 
}

long op_eval(char* op, long rand1, long rand2) 
{   
    if (strcmp(op, "+")  == 0) { return rand1 + rand2; }
    if (strcmp(op, "-")  == 0) { return rand1 - rand2; }
    if (strcmp(op, "*")  == 0) { return rand1 * rand2; }
    if (strcmp(op, "/")  == 0) { return rand1 / rand2; }
    if (strcmp(op, &pct) == 0) { return rand1 % rand2; }    
    if (strcmp(op, "^")  == 0) { return (long)pow(rand1, rand2); }

    return 0; 
}

long eval(mpc_ast_t* t) 
{
    if (strstr(t->tag, "number")) 
        return atoi(t->contents); 
    
    char* op = t->children[1]->contents;

    long result = eval(t->children[2]);

    int i = 3; 
    while ( strstr(t->children[i]->tag, "expr") )
    {
        result = op_eval(op, result, eval(t->children[i])); 
        i++;  
    }

    return result; 
}

int main(int argc, char** argv) 
{   
    // Polish Notation
    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Lip      = mpc_new("lip");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                        \
            number    : /-?[0-9]+/ ;                             \
            operator  : '+' | '-' | '*' | '/' | '%' | '^';       \
            expr      : <number> | '(' <operator> <expr>+ ')' ;  \
            lip       : /^/ <operator> <expr>+ /$/ ;             \
        ",
        Number, Operator, Expr, Lip);
    
    puts("Lips - Version 0.0.0.0.1"); 
    puts("Press ctrl+c to exit\n"); 
   
    while(1) 
    {
        char* input = readline("Lips \U0001F48B > ");
        add_history(input); 

        mpc_result_t result;
        if (mpc_parse("<stdin>", input, Lip, &result))
        {   
            // print the AST
            // mpc_ast_print(result.output);

            // traverse AST and print number of nodes 
            // int nodes_num = number_of_nodes(result.output); 
            // printf("Number of nodes: %i\n", nodes_num); 

            long eval_result = eval(result.output);
            printf("%li\n", eval_result);
            mpc_ast_delete(result.output);
        } 
        else 
        {
            mpc_err_print(result.error); 
            mpc_err_delete(result.error); 
        } 

        free(input); 
    }   

    mpc_cleanup(4, Number, Operator, Expr, Lip);
    return 0; 
}

