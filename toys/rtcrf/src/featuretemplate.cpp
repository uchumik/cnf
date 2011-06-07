# include "featuretemplate.hpp"

using namespace std;
using namespace sequential;
ftemplate::ftemplate(const char *tmpl)
: tmplfile(tmpl)
{
   this->lexer = new tmpllexer;
}

ftemplate::~ftemplate()
{
   delete this->lexer;
}

int ftemplate::parse(Sequence& sq,
      int current,
      fsetv_t& fv)
{
   this->in = new fstream(this->tmplfile.c_str(), ios::in);
   this->lexer->switch_streams(this->in, &cout);
   int ret = ftmpl_parser_parse(this, sq, current, fv);
   delete this->in;
   return ret;
}

int ftemplate::lex(void *retv)
{
   return this->lexer->yylex((YYSTYPE*)retv);
}

// tokenizer
int ftmpl_parser_lex(YYSTYPE *yylprm, void *parser)
{
   ftemplate *ft = (ftemplate*)parser;
   return ft->lex(yylprm);
}

// error message
void ftmpl_parser_error(const char *s)
{
   fprintf(stderr, "ERR: %s\n", s);
}

