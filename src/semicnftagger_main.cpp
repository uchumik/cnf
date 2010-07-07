# include <semicnftagger.h>
# include <cstdio>

int main(int argc, char **argv)
{
   SemiCnftagger tagger(*(argv+1), 5000000);
   tagger.read(*(argv+2));
   tagger.tagging(*(argv+3));
   return 0;
}
