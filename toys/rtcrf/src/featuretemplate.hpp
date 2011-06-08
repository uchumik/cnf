# ifndef __FEATURE_TMPL__
# define __FEATURE_TMPL__

# include <cstdio>
# include <cstdlib>
# include <string>
# include <vector>
# include <fstream>
# include "ftmpllexer.hpp"
# include "featuretemplate.tab.hpp"
# include "sequence.h"

struct realfeature_
{
   std::string key;
   unsigned int ngram; // unigram:1 bigram:2
   unsigned int id;
   double val;
};

typedef std::vector< realfeature_ > featureset_t;
typedef std::vector< featureset_t > fsetv_t;

static void fsetvdump(fsetv_t& fv)
{
   unsigned int size = fv.size();
   for (unsigned int i = 0; i < size; ++i)
   {
      unsigned int fsize = fv[i].size();
      for (unsigned int j = 0; j < fsize; ++j)
      {
         std::cout << fv[i][j].key
            << ':'
            << fv[i][j].val
            << '\t';
      }
      std::cout << std::endl;
   }
}

class ftemplate
{
   public:
      ftemplate(const char *file);
      ~ftemplate();
      int parse(sequential::Sequence& sq, int current, fsetv_t& fv);
      int lex(void *retv);
   protected:
      std::string tmplfile;
      std::fstream *in;
   private:
      ftemplate();
      ftemplate(const ftemplate&);
      ftemplate operator=(const ftemplate&);
      tmpllexer *lexer;
};

extern int ftmpl_parser_parse(void *parser, sequential::Sequence& sq, int current, fsetv_t& fv);
extern int ftmpl_parser_lex(YYSTYPE *yylprm, void *parser);

# endif /* __FEATURE_TMPL__ */
