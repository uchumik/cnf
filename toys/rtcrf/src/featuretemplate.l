%{
# include <iostream>
# include <cstring>

void yyerror(const char *massage);

# include "ftmpllexer.hpp"
# include "featuretemplate.tab.hpp"
# define YY_DECL int yyFlexLexer::yylex() { return 0; }\
   int tmpllexer::yylex(YYSTYPE *yylprm)
extern YYSTYPE yylval;

// yyFlexLexer::yywrap() is not implemented in Flex 2.5.35, thus implement it here.
int ftmpl_parser_FlexLexer::yywrap(){ return 1; }
%}
%option noyywrap
%option c++
%%

^#.+  {
   return COMMENTOUT;
}

^[UB][0-9]* {
   strcpy(yylprm->prefix,yytext);
   return PREFIX;
}

-?[0-9]+ {
   yylprm->pos = atoi(yytext);
   return POS;
}

-?[0-9]+\.[0-9]+   {
   yylprm->val = atof(yytext);
   return NUM;
}

":"   {
   return ':';
}

"%x"  {
   return BMACRO;
}

"%r"  {
   return RMACRO;
}

"+"   {
   return '+';
}

"-"   {
   return '-';
}

"*"   {
   return '*';
}

"/"   {
   return '/';
}

"%"   {
   return '%';
}

","   {
   return ',';
}

"["   {
   return '[';
}

"]"   {
   return ']';
}

"("   {
   return '(';
}

")"   {
   return ')';
}

"SUM" {
   return SUM;
}

"PROD"   {
   return PROD;
}

"MAX" {
   return MAX;
}

"MIN" {
   return MIN;
}

"LOG" {
   return LOG;
}

"EXP" {
   return EXP;
}

"SQRT"   {
   return SQRT;
}

"."   {
   return WILDCARD;
}

">"   {
   return GT;
}

">="  {
   return GE;
}

"<"   {
   return LT;
}

"<="  {
   return LE;
}
"=="  {
   return EQ;
}

[ \t] {
   /* do nothing */
}

"\n"  {
   return NL;
}

[^-+*%=<>:,\[\]]  {
   strcpy(yylprm->prefix,yytext);
   return SYMBOL;
}

.  {
   return yytext[0];
}
