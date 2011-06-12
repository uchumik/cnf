%{
# include <cstdio>
# include <cstdlib>
# include <cstring>
# include <vector>
# include <string>
# include <cmath>

# define YYPARSE_PARAM ftemplate, sequential::Sequence& sq, int current, fsetv_t& fv
# define YYLEX_PARAM ftemplate

# include "featuretemplate.hpp"
# include "featuretemplate.tab.hpp"
# include "sequence.h"


# define PREFIXBUFSIZE 4096

int ftmpl_parser_lex(YYSTYPE*, void*);
void ftmpl_parser_error(const char *s);
%}

%union
{
   char prefix[4096];
   double val;
   int pos;
}
%pure_parser
%token <prefix> PREFIX;
%token <val> BMACRO
%token <val> RMACRO
%token <val> NUM
%token <pos> POS
%token SUM
%token PROD
%token <val> MAX
%token <val> MIN
%token LOG
%token SQRT
%token EXP
%token WILDCARD
%token NL
%token COMMENTOUT
%token <prefix> SYMBOL
%token GT
%token GE
%token LT
%token LE
%token EQ
%left '+' '-'
%left '*' '/'
%left LOG SQRT EXP MAX MIN
%right UMINUS
%type <val> VAL
%type <prefix> BM
%type <val> RM
%type <val> MAXVALLIST
%type <val> MINVALLIST
%type <val> SUMLIST
%type <val> PRODLIST
//%type <pos>

%%
S
:  S TMPL NL
|  NL
|  /* empty */
;

TMPL
:  PREFIX ':' BM
{
   realfeature_ f;
   f.key = $<prefix>1;
   f.key += ":";
   f.key += $<prefix>3;
   ($<prefix>1[0] == 'U')?f.ngram=1:f.ngram=2;
   f.val = 1.0;
   fv[current].push_back(f);
}
|  PREFIX ':' RM
{
   if ($<val>3 != 0.)
   {
      realfeature_ f;
      f.key = $<prefix>1;
      ($<prefix>1[0] == 'U')?f.ngram=1:f.ngram=2;
      f.val = $<val>3;
      fv[current].push_back(f);
   }
}
|  PREFIX
{
   realfeature_ f;
   f.key = $<prefix>1;
   ($<prefix>1[0] == 'U')?f.ngram=1:f.ngram=2;
   f.val = 1.0;
   fv[current].push_back(f);
}
|  COMMENTOUT
{
   /* do nothing */
}
|  /* empty */
;

BM
: BM SYMBOL BM
{
   $<prefix>$[0] = '\0';
   strcat($<prefix>$,$<prefix>1);
   strcat($<prefix>$,$<prefix>2);
   strcat($<prefix>$,$<prefix>3);
}
| BMACRO '[' POS ',' POS ']'
{
   $<prefix>$[0] = '\0';
   strcpy($<prefix>$,sq.getToken(current+$<pos>3,$<pos>5));
}
|
{
   /* do nothing */
}
;

RM
: RM '+' RM
{
   $<val>$ = $<val>1 + $<val>3;
}
| RM '-' RM
{
   $<val>$ = $<val>1 - $<val>3;
}
| RM '*' RM
{
   $<val>$ = $<val>1 * $<val>3;
}
| RM '/' RM
{
   $<val>$ = $<val>1 / $<val>3;
}
| RM '%' RM
{
   $<val>$ = fmod($<val>1,$<val>3);
}
| '-' RM %prec UMINUS
{
   $<val>$ = -$<val>2;
}
| '(' RM ')'
{
   $<val>$ = $<val>2;
}
| LOG RM
{
   $<val>$ = log($<val>2);
}
| SQRT RM
{
   $<val>$ = sqrt($<val>2);
}
| EXP RM
{
   $<val>$ = exp($<val>2);
}
| MAX '(' MAXVALLIST ')'
{
   $<val>$ = $<val>3;
}
| MIN '(' MINVALLIST ')'
{
   $<val>$ = $<val>3;
}
| SUM '(' SUMLIST ')'
{
   $<val>$ = $<val>3;
}
| PROD '(' PRODLIST ')'
{
   $<val>$ = $<val>3;
}
| RM GT RM
{
   $<val>$ = ($<val>1 > $<val>3)?1:0;
}
| RM GE RM
{
   $<val>$ = ($<val>1 >= $<val>3)?1:0;
}
| RM LT RM
{
   $<val>$ = ($<val>1 < $<val>3)?1:0;
}
| RM LE RM
{
   $<val>$ = ($<val>1 <= $<val>3)?1:0;
}
| RM EQ RM
{
   $<val>$ = ($<val>1 == $<val>3)?1:0;
}
| VAL
{
   $<val>$ = $<val>1;
}
;

MAXVALLIST
: MAXVALLIST ',' MAXVALLIST
{
   $<val>$ = ($<val>1 > $<val>3) ? $<val>1:$<val>3;
}
| RM
{
   $<val>$ = $<val>1;
}
| RMACRO '[' POS ',' WILDCARD ']'
{
   bool init = false;
   unsigned int col = sq.getColSize();
   for (unsigned int j = 0; j < col-1; ++j)
   {
      char *t = sq.getToken(current+$<pos>3,j);
      char *e;
      double c = strtod(t,&e);
      if (!init && t != e)
      {
         $<val>$ = c;
         init = true;
      }
      else if (init && t != e)
      {
         $<val>$ = ($<val>$ > c)?$<val>$:c;
      }
   }
}
| RMACRO '[' WILDCARD ',' POS ']'
{
   char *t = sq.getToken(0,$<pos>5);
   char *e;
   $<val>$ = strtod(t,&e);
   if (t != e)
   {
      unsigned int row = sq.getRowSize();
      for (unsigned int i = 1; i < row; ++i)
      {
         t = sq.getToken(i,$<pos>5);
         double c = strtod(t,&e);
         if (t != e)
         {
            $<val>$ = ($<val>$ > c)?$<val>$:c;
         }
      }
   }
}
;

MINVALLIST
: MINVALLIST ',' MINVALLIST
{
   $<val>$ = ($<val>1 < $<val>3) ? $<val>1:$<val>3;
}
| RM
{
   $<val>$ = $<val>1;
}
| RMACRO '[' WILDCARD ',' POS ']'
{
   char *t = sq.getToken(0,$<pos>5);
   char *e;
   $<val>$ = strtod(t,&e);
   if (t != e)
   {
      unsigned int row = sq.getRowSize();
      for (unsigned int i = 1; i < row; ++i)
      {
         t = sq.getToken(i,$<pos>5);
         double c = strtod(t,&e);
         if (t != e)
         {
            $<val>$ = ($<val>$ < c)?$<val>$:c;
         }
      }
   }
}
| RMACRO '[' POS ',' WILDCARD ']'
{
   bool init = false;
   unsigned int col = sq.getColSize();
   for (unsigned int j = 0; j < col-1; ++j)
   {
      char *t = sq.getToken(current+$<pos>3,j);
      char *e;
      double c = strtod(t,&e);
      if (!init && t != e)
      {
         $<val>$ = c;
         init = true;
      }
      else if (init && t != e)
      {
         $<val>$ = ($<val>$ < c)?$<val>$:c;
      }
   }
}
;

SUMLIST
: SUMLIST ',' SUMLIST
{
   $<val>$ = $<val>1 + $<val>3;
}
| RM
{
   $<val>$ = $<val>1;
}
| RMACRO '[' WILDCARD ',' POS ']'
{
   $<val>$ = 0.;
   unsigned int row = sq.getRowSize();
   for (unsigned int i = 0; i < row; ++i)
   {
      $<val>$ += atof(sq.getToken(i,$<pos>5));
   }
}
| RMACRO '[' POS ',' WILDCARD ']'
{
   $<val>$ = 0.;
   unsigned int col = sq.getColSize();
   for (unsigned int j = 0; j < col-1; ++j)
   {
      char *t = sq.getToken(current+$<pos>3,j);
      char *e;
      double c = strtod(t,&e);
      if (t != e)
      {
         $<val>$ += c;
      }
   }
}
| RMACRO '[' WILDCARD ',' WILDCARD ']'
{
   $<val>$ = 0.;
   unsigned int row = sq.getRowSize();
   unsigned int col = sq.getColSize();
   for (unsigned int i = 0; i < row; ++i)
   {
      // j == col-1 then j points label col
      for (unsigned int j = 0; j < col-1; ++j)
      {
         char *t = sq.getToken(i,j);
         char *e;
         double c = strtod(t,&e);
         if (t != e)
         {
            $<val>$ += c;
         }
      }
   }
}
;

PRODLIST
: PRODLIST ',' PRODLIST
{
   $<val>$ = $<val>1 * $<val>3;
}
| RM
{
   $<val>$ = $<val>1;
}
| RMACRO '[' WILDCARD ',' POS ']'
{
   $<val>$ = 1.;
   unsigned int row = sq.getRowSize();
   for (unsigned int i = 0; i < row; ++i)
   {
      $<val>$ *= atof(sq.getToken(i,$<pos>5));
   }
}
| RMACRO '[' POS ',' WILDCARD ']'
{
   $<val>$ = 1.;
   unsigned int col = sq.getColSize();
   for (unsigned int j = 0; j < col-1; ++j)
   {
      char *t = sq.getToken(current+$<pos>3,j);
      char *e;
      double c = strtod(t,&e);
      if (t != e)
      {
         $<val>$ *= c;
      }
   }
}
| RMACRO '[' WILDCARD ',' WILDCARD ']'
{
   $<val>$ = 1.;
   unsigned int row = sq.getRowSize();
   unsigned int col = sq.getColSize();
   for (unsigned int i = 0; i < row; ++i)
   {
      // j == col-1 then j points label col
      for (unsigned int j = 0; j < col-1; ++j)
      {
         char *t = sq.getToken(i,j);
         char *e;
         double c = strtod(t,&e);
         if (t != e)
         {
            $<val>$ *= c;
         }
      }
   }
}
;

VAL
: RMACRO '[' POS ',' POS ']'
{
   //char *p = sq.getToken(current+$<pos>3,$<pos>5);
   $<val>$ = atof(sq.getToken(current+$<pos>3,$<pos>5));

}
| NUM
{
   $<val>$ = $<val>1;
}
;

