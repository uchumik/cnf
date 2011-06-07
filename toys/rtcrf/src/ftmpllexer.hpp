# ifndef __FTMPL_LEXER__
# define __FTMPL_LEXER__

# include "featuretemplate.tab.hpp"
# undef yyFlexLexer
# define yyFlexLexer ftmpl_parser_FlexLexer
# include <FlexLexer.h>

class tmpllexer : public yyFlexLexer
{
   public:
      tmpllexer( std::istream* arg_yyin = 0,
            std::ostream* arg_yyout = 0 ) : yyFlexLexer( arg_yyin, arg_yyout ){}
      ~tmpllexer(){}
      int yylex(YYSTYPE *yylprm);
   protected:
   private:
};

# endif /* __FTMPL_LEXER__ */
