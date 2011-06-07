# include "featuretemplate.hpp"
# include "sequence.h"
# include "myutil.h"

# define BUFSIZE 4096

using namespace std;
using namespace sequential;
using namespace toyutil;

int main(int argc, char **argv)
{
   ftemplate ft(*(argv+1));

   FILE *fp = NULL;
   if ((fp = fopen(*(argv+2),"r")) == NULL)
   {
      fprintf(stderr,"Couldn't open %s\n",*(argv+2));
      return 1;
   }
   char buf[BUFSIZE];
   Sequence sq;
   sq.setColSize(6);
   sq.init();
   while (feof(fp) == 0)
   {
      MyUtil::sqread(fp, &sq, BUFSIZE);
      if (sq.getRowSize() == 0)
      {
         continue;
      }
      fsetv_t fv;
      unsigned int row = sq.getRowSize();
      fv.resize(row);
      for (unsigned int i = 0; i < row; ++i)
      {
         ft.parse(sq, i, fv);
      }
      sq.dump();
      fsetvdump(fv);
      sq.clear();
   }
   fclose(fp);

   return 0;
}
