%{
#include <stdio.h>
#include <string.h>
#include "typecheck.hh"
#include "node.hh"
#include "codegen.hh"
void yyerror(char *);


extern int yylex();

// Global variables defined by lex.yy.c.
extern int yylineno;
extern char* yytext;
extern FILE *yyin;

struct ASTNode* root;
%}

// Declares all variants of semantic values. Yacc/Bison copies all variants
// to the generated header file (y.tab.h) enclosed in a C-language union
// declaration, named `YYSTYPE`. Check out the header file to see it.
%union {
    struct ASTNode* node;
    int integer;
    char * string;
}



// Left hand non-terminals. They are all associated to the `node` variant
// declared in the %union section, which is of type `ASTNode *`.
%type <node> Program MainClass StaticVarDeclList StaticMethodDeclList StatementList MainMethod BracketList
%type <node> VarDecl Init idExpList StaticVarDecl StaticMethodDecl FormalList TypeIDList PrimeType
%type <node> Type Statement MethodCall Exp ExpList commaExpList LeftValue BracketExpList EmptyAction





// Declares tokens. In the generated y.tab.h file, each token gets declared as 
// a enum constant and assigned with a unique number. These enum constants are
// used in the lex file, returned by `yylex()` to denote the symbolic tokens.

// These keyword-like tokens doesn't need to have a semantic value.
%token KW_BOOLEAN KW_CLASS KW_FALSE KW_TRUE KW_INT MAIN KW_PUBLIC KW_VOID 
%token KW_STATIC KW_STRING SYSTEM_OUT_PRINT SYSTEM_OUT_PRINTLN KW_RETURN
%token KW_IF KW_ELSE KW_WHILE KW_NEW KW_LENGTH KW_PRIVATE INTEGER_PARSEINT


// These tokens have additional information aside from what kind of token it
// is, so they carry semantic information.
%token <integer> INTEGER_LITERAL
%token <string> STRING_LITERAL ID

%left TOK_OR
%left TOK_AND 
%left TOK_EQ TOK_NEQ
%left '<' '>' TOK_LTE TOK_GTE
%left '+' '-'
%left '*' '/'
%right '!'
%start Program

%%

Program:                
    MainClass {   
        $$ = new_node(NODETYPE_PROGRAM, yylineno);
        root = $$;
        add_child($$, $1);
    };

MainClass: 
    KW_CLASS ID '{' StaticVarDeclList StaticMethodDeclList MainMethod '}' {
        $$ = new_node(NODETYPE_MAINCLASS, yylineno);
        $$->has_curly_braces = true;
        set_string_value($$, $2);
        if ($4 != NULL) add_child($$, $4);
        if ($5 != NULL) add_child($$, $5);
        add_child($$, $6);
    };

MainMethod:
    KW_PUBLIC KW_STATIC KW_VOID MAIN '(' KW_STRING '[' ']' ID ')' '{' StatementList '}' {
        $$ = new_node(NODETYPE_MAINMETHOD, yylineno);
        set_string_value($$, "main");
        $$->has_curly_braces = true;
        add_child($$, $12);
    }
    | KW_PUBLIC KW_STATIC KW_VOID MAIN '(' KW_STRING '[' ']' ID ')' '{''}' {
        $$ = new_node(NODETYPE_MAINMETHOD, yylineno);
        $$->has_curly_braces = true;
    };

StaticVarDeclList:
    StaticVarDeclList StaticVarDecl {
        $$ = new_node(NODETYPE_STATICVARDECLLIST, yylineno);
        if ($1 != NULL) add_child($$, $1);
        add_child($$, $2);
    }
    | EmptyAction
    ;
StaticMethodDeclList:
    StaticMethodDeclList StaticMethodDecl {
        $$ = new_node(NODETYPE_STATICMETHODDECLLIST, yylineno);
        if ($1 != NULL) add_child($$, $1);
        add_child($$, $2);
    }
    | EmptyAction
    ;

StatementList:
    StatementList Statement {
        $$ = new_node(NODETYPE_STATEMENTLIST, yylineno);
        add_child($$, $1);
        add_child($$, $2);
    }
    | Statement {
        $$ = new_node(NODETYPE_STATEMENTLIST, yylineno);
        add_child($$, $1);
    };
VarDecl:
    Type ID Init idExpList ';' {
        $$ = new_node(NODETYPE_VARDECL, yylineno);
        set_string_value($$, $2);
        add_child($$, $1);
        if ($3 != NULL) add_child($$, $3);
        add_child($$, $4);
    }
    | Type ID Init ';' {
        $$ = new_node(NODETYPE_VARDECL, yylineno);
        set_string_value($$, $2);
        add_child($$, $1);
        if ($3 != NULL) add_child($$, $3);
    };
idExpList:
    idExpList ',' ID Init  {
        $$ = new_node(NODETYPE_IDEXPLIST, yylineno);
        set_string_value($$, $3);
        add_child($$, $1);
        if ($4 != NULL) add_child($$, $4);
    }
    | ',' ID Init {
        $$ = new_node(NODETYPE_IDEXPLIST, yylineno);
        set_string_value($$, $2);
        if ($3 != NULL) add_child($$, $3);
    };
Init:
    '=' Exp {
        $$ = new_node(NODETYPE_INIT, yylineno);
        add_child($$, $2);
    }
    | EmptyAction
    ;
StaticVarDecl:
    KW_PRIVATE KW_STATIC VarDecl {
        $$ = new_node(NODETYPE_STATICVARDECL, yylineno);
        add_child($$, $3);
        $3->is_static = true;
    };
StaticMethodDecl:
    KW_PUBLIC KW_STATIC Type ID '(' FormalList ')' '{' StatementList '}' {
        $$ = new_node(NODETYPE_STATICMETHODDECL, yylineno);
        $$->has_curly_braces = true;
        set_string_value($$, $4);
        add_child($$, $3);
        if ($6 != NULL) add_child($$, $6);
        add_child($$, $9);
    }
    | KW_PUBLIC KW_STATIC Type ID '(' FormalList ')' '{''}' {
        $$ = new_node(NODETYPE_STATICMETHODDECL, yylineno);
        $$->has_curly_braces = true;
        set_string_value($$, $4);
        add_child($$, $3);
        if ($6 != NULL) add_child($$, $6);
    };

FormalList:
    Type ID TypeIDList {
        $$ = new_node(NODETYPE_FORMALLIST, yylineno);
        set_string_value($$, $2);
        add_child($$, $1);
        add_child($$, $3);
    }
    | Type ID {
        $$ = new_node(NODETYPE_FORMALLIST, yylineno);
        set_string_value($$, $2);
        add_child($$, $1);
    }
    | EmptyAction 
    ;

TypeIDList:
    ',' Type ID TypeIDList {
        $$ = new_node(NODETYPE_TYPEIDLIST, yylineno);
        set_string_value($$, $3);
        add_child($$, $2);
        add_child($$, $4);
    }
    | ',' Type ID {
        $$ = new_node(NODETYPE_TYPEIDLIST, yylineno);
        set_string_value($$, $3);
        add_child($$, $2);
    };

PrimeType:                   
    KW_INT {
        $$ = new_node(NODETYPE_PRIMETYPE, yylineno);
        $$ -> data.type = DATATYPE_INT;
    }
    | KW_BOOLEAN {
        $$ = new_node(NODETYPE_PRIMETYPE, yylineno);
        $$ -> data.type = DATATYPE_BOOLEAN;
    }
    | KW_STRING {
        $$ = new_node(NODETYPE_PRIMETYPE, yylineno);
        $$ -> data.type = DATATYPE_STR;
    };
Type:
    PrimeType {
        $$ = new_node(NODETYPE_TYPE, yylineno);
        add_child($$, $1);
    }
    | PrimeType '[' ']' {
        $$ = new_node(NODETYPE_TYPE_ONE_ARR, yylineno);
        add_child($$, $1);
    }
    | PrimeType '[' ']' '[' ']' {
        $$ = new_node(NODETYPE_TYPE_TWO_ARR, yylineno);
        add_child($$, $1);
    }
    | PrimeType '[' ']' '[' ']' BracketList
    ;
                    
BracketList:
    BracketList '[' ']' {
        
    }
    | '[' ']' {
       
    };

Statement:              
    VarDecl {
        $$ = new_node(NODETYPE_STATEMENT, yylineno);
        add_child($$, $1);
    }
    | '{' StatementList '}' {
        $$ = new_node(NODETYPE_STATEMENTLIST, yylineno);
        $$->has_curly_braces = true;
        add_child($$, $2);
    }
    | '{' '}' {
        $$ = new_node(NODETYPE_STATEMENT, yylineno);
        $$->has_curly_braces = true;
    }
    | KW_IF '(' Exp ')' Statement KW_ELSE Statement {
        $$ = new_node(NODETYPE_IF, yylineno);
        add_child($$, $3);
        add_child($$, $5);
        add_child($$, $7);
    }
    | KW_WHILE '(' Exp ')' Statement {
        $$ = new_node(NODETYPE_WHILE, yylineno);
        add_child($$, $3);
        add_child($$, $5);
    }
    | SYSTEM_OUT_PRINTLN '(' Exp ')' ';' {
        $$ = new_node(NODETYPE_PRINTLN, yylineno);
        add_child($$, $3);
    }
    | SYSTEM_OUT_PRINT '(' Exp ')' ';' {
        $$ = new_node(NODETYPE_PRINT, yylineno);
        add_child($$, $3);
    }
    | LeftValue '=' Exp ';' {
        $$ = new_node(NODETYPE_ASSIGN, yylineno);
        add_child($$, $1);
        add_child($$, $3);
    }
    | KW_RETURN Exp ';' {
        $$ = new_node(NODETYPE_RETURN, yylineno);
        add_child($$, $2);
    }
    | MethodCall ';' {
        $$ = new_node(NODETYPE_STATEMENT_METHODCALL, yylineno);
        add_child($$, $1);
    };
MethodCall: 
    ID '(' ExpList ')' {
        $$ = new_node(NODETYPE_METHODCALL, yylineno);
        set_string_value($$, $1);
        if ($3 != NULL) add_child($$, $3);
    }
    | INTEGER_PARSEINT '(' Exp ')' {
        $$ = new_node(NODETYPE_PARSEINT, yylineno);
        add_child($$, $3);
    }
    | MAIN '(' ExpList ')' {
        $$ = new_node(NODETYPE_CALL_MAIN, yylineno);
        set_string_value($$, "main");
        if ($3 != NULL) add_child($$, $3);
    }
    ;

Exp:
    Exp '+' Exp {
        $$ = new_node(NODETYPE_EXP_PLUS, yylineno);
        add_child($$, $1);
        add_child($$, $3);
    }
    | Exp '-' Exp {
        $$ = new_node(NODETYPE_EXP_OPERATION, yylineno);
        add_child($$, $1);
        add_child($$, $3);
        set_string_value($$, "-");
    }
    | Exp '*' Exp {
        $$ = new_node(NODETYPE_EXP_OPERATION, yylineno);
        add_child($$, $1);
        add_child($$, $3);
        set_string_value($$, "*");
    }
    | Exp '/' Exp {
        $$ = new_node(NODETYPE_EXP_OPERATION, yylineno);
        add_child($$, $1);
        add_child($$, $3);
    }
    | Exp '<' Exp {
        $$ = new_node(NODETYPE_EXP_OPERATION_COMP, yylineno);
        add_child($$, $1);
        add_child($$, $3);
        set_string_value($$, "<");
    }
    | Exp '>' Exp {
        $$ = new_node(NODETYPE_EXP_OPERATION_COMP, yylineno);
        add_child($$, $1);
        add_child($$, $3);
        set_string_value($$, ">");
    }
    | Exp TOK_LTE Exp {
        $$ = new_node(NODETYPE_EXP_OPERATION_COMP, yylineno);
        add_child($$, $1);
        add_child($$, $3);
        set_string_value($$, "<=");

    }
    | Exp TOK_GTE Exp {
        $$ = new_node(NODETYPE_EXP_OPERATION_COMP, yylineno);
        add_child($$, $1);
        add_child($$, $3);
        set_string_value($$, ">=");

    }
    | Exp TOK_AND Exp {
        $$ = new_node(NODETYPE_EXP_LOGIC, yylineno);
        add_child($$, $1);
        add_child($$, $3);
        set_string_value($$, "&&");
    }
    | Exp TOK_OR Exp {
        $$ = new_node(NODETYPE_EXP_LOGIC, yylineno);
        add_child($$, $1);
        add_child($$, $3);
        set_string_value($$, "||");
    }
    | Exp TOK_EQ Exp {
        $$ = new_node(NODETYPE_EXP_COMP, yylineno);
        add_child($$, $1);
        add_child($$, $3);
        set_string_value($$, "==");
    }
    | Exp TOK_NEQ Exp {
        $$ = new_node(NODETYPE_EXP_COMP, yylineno);
        add_child($$, $1);
        add_child($$, $3);
        set_string_value($$, "!=");
    }
    | '!' Exp {
        $$ = new_node(NODETYPE_EXP_LOGIC_NOT, yylineno);
        add_child($$, $2);
        
    }
    | '+' Exp {
        $$ = new_node(NODETYPE_EXP_POS, yylineno);
        add_child($$, $2);
        
    }
    | '-' Exp {
        $$ = new_node(NODETYPE_EXP_NEG, yylineno);
        add_child($$, $2);
    }
    |'(' Exp ')' { 
        $$ = new_node(NODETYPE_EXP, yylineno);
        add_child($$, $2);
    }
    | LeftValue {
        $$ = new_node(NODETYPE_EXP_LEFT, yylineno);
        add_child($$, $1);
    }
    | LeftValue '.' KW_LENGTH {
        $$ = new_node(NODETYPE_EXP_LENGTH, yylineno);
        add_child($$, $1);
    }
    | INTEGER_LITERAL {
        $$ = new_node(NODETYPE_EXP_TYPE, yylineno);
        set_int_value($$, $1);
    }
    | STRING_LITERAL {
        $$ = new_node(NODETYPE_EXP_TYPE, yylineno);
        set_string_value($$, $1);
    }
    | KW_TRUE {
        $$ = new_node(NODETYPE_EXP_TYPE, yylineno);
        set_boolean_value($$, true);
    }
    | KW_FALSE {
        $$ = new_node(NODETYPE_EXP_TYPE, yylineno);
        set_boolean_value($$, false);
    }
    | MethodCall { //Todo 
        $$ = new_node(NODETYPE_EXP_METHODCALL, yylineno);
        add_child($$, $1);
    }
    | KW_NEW PrimeType '[' Exp ']' { //Todo
        $$ = new_node(NODETYPE_EXP_ONE_ARR, yylineno);
        add_child($$, $2);
        add_child($$, $4);
    }
    | KW_NEW PrimeType '[' Exp ']' '[' Exp ']' { //Todo
        $$ = new_node(NODETYPE_EXP_TWO_ARR, yylineno);
        add_child($$, $2);
        add_child($$, $4);
        add_child($$, $7);
    }
    | KW_NEW PrimeType '[' Exp ']' '[' Exp ']' BracketExpList { 
        $$ = new_node(NODETYPE_EXP_ARR, yylineno);
    } 
    ;
ExpList:
    Exp commaExpList {
        $$ = new_node(NODETYPE_EXPLIST, yylineno);
        add_child($$, $1);
        add_child($$, $2);
    }
    | Exp {
        $$ = new_node(NODETYPE_EXPLIST, yylineno);
        add_child($$, $1);
    }
    | EmptyAction
    ;
commaExpList:
    ',' Exp commaExpList {
        $$ = new_node(NODETYPE_COMMAEXPLIST, yylineno);
        add_child($$, $2);
        add_child($$, $3);
    }
    | ',' Exp {
        $$ = new_node(NODETYPE_COMMAEXPLIST, yylineno);
        add_child($$, $2);
    };
LeftValue:
    ID {
        $$ = new_node(NODETYPE_LEFTVALUE_ID, yylineno);
        set_string_value($$, $1);
    }
    | ID '[' Exp ']' {
        $$ = new_node(NODETYPE_LEFTVALUE_ONE_ARR, yylineno);
        set_string_value($$, $1);
        add_child($$, $3);
    }
    | ID '[' Exp ']' '[' Exp ']' {
        $$ = new_node(NODETYPE_LEFTVALUE_TWO_ARR, yylineno);
        set_string_value($$, $1);
        add_child($$, $3);
        add_child($$, $6);
    }
    | ID '[' Exp ']' '[' Exp ']' BracketExpList {
        $$ = new_node(NODETYPE_LEFTVALUE_ARR, yylineno);
    }
    ;
BracketExpList:
    '[' Exp ']' BracketExpList {
        $$ = new_node(NODETYPE_BRACKETEXPLIST, yylineno);
    }
    | '[' Exp ']' {
        $$ = new_node(NODETYPE_BRACKETEXPLIST, yylineno);
    }
    ;
EmptyAction:
    {
        $$ = NULL;
    }
    ;

%%


void yyerror(char* s) {
    fprintf(stderr, "Syntax errors in line %d\n", yylineno);
}


int main(int argc, char* argv[] )
{
    yyin = fopen( argv[1], "r" );

    // Checks for syntax errors and constructs AST
    if (yyparse() != 0)
        return 1;

    // Traverse the AST to check for semantic errors if no syntac errors
    print_tree_with_lines(root, 0);
    traverse_and_fill_table(root);
    traverse_and_check_type(root);
    checkProgram(root);
    if (has_error())return 1;
    traverse_and_codegen(root, false);
    traverse_and_codegen(root, true);
    char filename[50];
    strncpy(filename, argv[1], strlen(argv[1]) - 5);
    filename[strlen(argv[1]) - 5] = '\0';
    generateCode(filename);
    
    return 0;
}
