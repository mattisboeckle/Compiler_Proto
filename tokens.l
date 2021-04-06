%{
#include <string>
#include "node.h"
#include "parser.hpp"
#define SAVE_TOKEN yylval.string = new std::string(yytext, yyleng)
#define SAVE_STRING yylval.string = new std::string(yytext, 1, yyleng-2);
#define TOKEN(t) (yylval.token = t)

void save_string() {
    if(yyleng>2) {
        SAVE_STRING;
    }else {
        SAVE_TOKEN;
    }
}

// \"[a-zA-Z0-9_]*\"           save_string(); return STRING;
// \".*\"				        SAVE_TOKEN; return STRING;

extern int Line;

%}

%option noyywrap

%%

[\n]                                    Line++;
[ \t]					                ;
"int"|"double"|"string"|"boolean"       SAVE_TOKEN; return TYPE;
"return"                                return TOKEN(RETURN);
"if"                                    return TOKEN(IF);
"else"                                  return TOKEN(ELSE);
"true"|"false"                          SAVE_TOKEN; return BOOLEAN;
[0-9]+\.[0-9]*		                    SAVE_TOKEN; return DOUBLE;
[0-9]*				                    SAVE_TOKEN; return INTEGER;
[a-zA-Z_][a-zA-Z0-9_]*		            SAVE_TOKEN; return IDENTIFIER;
"="                                     return TOKEN(EQUAL);
"=="                                    return TOKEN(CEQ);
"!="                                    return TOKEN(CNE);
"<"                                     return TOKEN(CLT);
"<="                                    return TOKEN(CLE);
">"                                     return TOKEN(CGT);
">="                                    return TOKEN(CGE);
"("                                     return TOKEN(LPAREN);
")"                                     return TOKEN(RPAREN);
"{"                                     return TOKEN(LBRACE);
"}"                                     return TOKEN(RBRACE);
"."                                     return TOKEN(DOT);
","                                     return TOKEN(COMMA);
"+"						                return TOKEN(PLUS);
"-"						                return TOKEN(MINUS);
"*" 					                return TOKEN(MUL);
"/"						                return TOKEN(DIV);
":"						                return TOKEN(COLON);
"::"                                    return TOKEN(DOUBLE_COLON);
";"						                return TOKEN(SEMICOLON);
.					                    printf("Unknown token!\n"); yyterminate();

%% 