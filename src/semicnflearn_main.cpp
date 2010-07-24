# include <semicnflearn.h>

using namespace SemiCnf;
int main(int argc, char **argv)
{
   SemiCnflearn learner(*(argv+1),*(argv+2), 15000000);
   learner.init();
   learner.learn(100,1);
   learner.save(*(argv+3));

   return 0;
}
