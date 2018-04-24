#include <stdio.h> 
#include <stdlib.h>
#include <editline/readline.h>
#include "mpc.h"

typedef struct lval 
{
    int type;
    long num;
    char* err;
    char* sym;
    int count; 
    struct lval** cell; //cell points to a location where we store a list of lval* 
} lval;

enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

static char mod = '%'; 

// construct a pointer to a new lval number
lval* lval_num (long x) 
{
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

// construct a pointer to a new lval error
lval* lval_err (char* m) 
{
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

// construct a pointer to a new lval symbol
lval* lval_sym (char* s) 
{
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

// construct a pointer to a new empty lval s-expression
lval* lval_sexpr(void) 
{
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

void lval_del(lval* v) 
{
    switch (v->type) 
    {
        /* Do nothing special for number type */
        case LVAL_NUM: break;

        /* For Err or Sym free the string data */
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;

        /* If Sexpr then delete all elements inside */
        case LVAL_SEXPR:
        for (int i = 0; i < v->count; i++) {
            lval_del(v->cell[i]);
        }
        /* Also free the memory allocated to contain the pointers */
        free(v->cell);
        break;
    }

    /* Free the memory allocated for the "lval" struct itself */
    free(v);
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

lval eval_op(char* op, lval rand1, lval rand2) 
{   

    if (rand1.type == LVAL_ERR) { return rand1; }
    if (rand2.type == LVAL_ERR) { return rand2; }


    if (strcmp(op, "+")  == 0) { return lval_num(rand1.num + rand2.num); }
    if (strcmp(op, "-")  == 0) { return lval_num(rand1.num - rand2.num); }
    if (strcmp(op, "*")  == 0) { return lval_num(rand1.num * rand2.num); }
    if (strcmp(op, &mod) == 0) { return lval_num(rand1.num % rand2.num); }    
    if (strcmp(op, "^")  == 0) { return lval_num( (long)pow(rand1.num, rand2.num) ); }
    if (strcmp(op, "/")  == 0) 
    {
        return rand2.num == 0 
            ? lval_err(LERR_DIV_ZERO) 
            : lval_num(rand1.num / rand2.num); 
    }

    return lval_err(LERR_BAD_OP); 
}

lval eval(mpc_ast_t* t) 
{
    if (strstr(t->tag, "number"))
    {
        errno = 0; 
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM); 
    } 
    
    char* op = t->children[1]->contents;

    lval result = eval(t->children[2]);

    int i = 3; 
    while ( strstr(t->children[i]->tag, "expr") )
    {
        result = eval_op(op, result, eval(t->children[i])); 
        i++;  
    }

    return result; 
}

void lval_print(lval v) 
{
  switch (v.type) 
  {

    case LVAL_NUM: 
        printf("%li", v.num); 
        break;

    case LVAL_ERR: 
        switch(v.err)
        {
        case LERR_DIV_ZERO: 
            printf("Error: Division By Zero!"); 
            break;
            
        case LERR_BAD_OP: 
            printf("Error: Invalid Operator!"); 
            break;
            
        case LERR_BAD_NUM: 
            printf("Error: Invalid Number!"); 
            break;
        }
    break;
  }
}

/* Print an "lval" followed by a newline */
void lval_println(lval v) { lval_print(v); putchar('\n'); }

int main(int argc, char** argv) 
{   
    // Polish Notation
    mpc_parser_t* Number   = mpc_new("number");
    mpc_parser_t* Symbol   = mpc_new("symbol");
    mpc_parser_t* Sexpr    = mpc_new("sexpr");
    mpc_parser_t* Expr     = mpc_new("expr");
    mpc_parser_t* Lip      = mpc_new("lip");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                        \
            number    : /-?[0-9]+/ ;                             \
            symbol    : '+' | '-' | '*' | '/' | '%' | '^';       \
            sexpr     : '(' <expr>* ')' ;                        \
            expr      : <number> | '(' <operator> <expr>+ ')' ;  \
            lip       : /^/ <operator> <expr>+ /$/ ;             \
        ",
        Number, Symbol, Sexpr, Expr, Lip);
    
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

            lval eval_result = eval(result.output);
            lval_println(eval_result);
            mpc_ast_delete(result.output);
        } 
        else 
        {
            mpc_err_print(result.error); 
            mpc_err_delete(result.error); 
        } 

        free(input); 
    }   

    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lip);
    return 0; 
}

// TODO(shaw): need  destructors to match the constructors of lval pointers to free memory claimed by malloc