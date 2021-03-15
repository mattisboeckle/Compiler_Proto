%{
    #include "node.h"
    #include <cstdio>
    #include <cstdlib>
    NBlock *programBlock; /* the top level root node of our final AST */

    int Line = 0;
    extern int yylex();
    void yyerror(const char *s) { printf("ERROR: %s | Line %d\n", s, Line); std::exit(1); }
%}

/* Represents the many different ways we can access our data */
%union {
    Node *node;
    NBlock *block;
    NExpression *expr;
    NStatement *stmt;
    NIdentifier *ident;
    NVariableDeclaration *var_decl;
    std::vector<NVariableDeclaration*> *varvec;
    std::vector<NExpression*> *exprvec;
    std::string *string;
    int token;
}

/* Define our terminal symbols (tokens). This should
   match our tokens.l lex file. We also define the node type
   they represent.
 */
%token <string> IDENTIFIER INTEGER DOUBLE TYPE BOOLEAN
%token <token> CEQ CNE CLT CLE CGT CGE EQUAL
%token <token> LPAREN RPAREN LBRACE RBRACE COMMA DOT COLON DOUBLE_COLON SEMICOLON
%token <token> PLUS MINUS MUL DIV
%token <token> RETURN PRINT

%type <ident> ident type
%type <expr> numeric expr
%type <varvec> func_decl_args
%type <exprvec> call_args
%type <block> program stmts block
%type <stmt> stmt var_decl func_decl
%type <token> comparison

/* Operator precedence for mathematical operators */
%left PLUS MINUS
%left MUL DIV

%start program

%%

/* New stuff */

program : stmts { programBlock = $1; }
        ;

stmts : stmt { $$ = new NBlock(); $$->statements.push_back($<stmt>1);}
      | stmts stmt { $1->statements.push_back($<stmt>2); }
      ;

stmt : func_decl 
     | var_decl SEMICOLON
     | expr SEMICOLON { $$ = new NExpressionStatement(*$1); }
     | RETURN expr SEMICOLON { $$ = new NReturnStatement(*$2);}
     ;

block : LBRACE stmts RBRACE { $$ = $2; }
      | LBRACE RBRACE { $$ = new NBlock(); }
      ;

var_decl    : ident COLON type { $$ = new NVariableDeclaration(*$3, *$1); }
            | ident COLON type EQUAL expr { $$ = new NVariableDeclaration(*$3, *$1, $5); }
            ;

func_decl : type DOUBLE_COLON ident LPAREN func_decl_args RPAREN block
            { $$ = new NFunctionDeclaration(*$1, *$3, *$5, *$7); }
            ;

func_decl_args : /*blank*/ { $$ = new VariableList(); }
                  | var_decl { $$ = new VariableList(); $$->push_back($<var_decl>1); }
                  | func_decl_args COMMA var_decl { $1->push_back($<var_decl>3); }
                  ;

type : TYPE { $$ = new NIdentifier(*$1); delete $1; }

ident : IDENTIFIER { $$ = new NIdentifier(*$1); delete $1; }
      ;

numeric : INTEGER { $$ = new NInteger(atol($1->c_str())); delete $1; }
        | DOUBLE  { $$ = new NDouble(atof($1->c_str())); delete $1; }
        | BOOLEAN { $$ = new NBoolean($1->c_str()); delete $1; }
        ;

expr : ident EQUAL expr { $$ = new NAssignment(*$<ident>1, *$3); }
     | ident LPAREN call_args RPAREN { $$ = new NMethodCall(*$1, *$3); delete $3; }
     | ident { $<ident>$ = $1; }
     | numeric
     | expr MUL expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
     | expr DIV expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
     | expr PLUS expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
     | expr MINUS expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
     | expr comparison expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
     | LPAREN expr RPAREN { $$ = $2; }
     ;

call_args : /*blank*/ { $$ = new ExpressionList(); }
          | expr { $$ = new ExpressionList(); $$->push_back($1); }
          | call_args COMMA expr { $1->push_back($3); }

comparison : CEQ | CNE | CLT | CLE | CGT | CGE
           ;

%%