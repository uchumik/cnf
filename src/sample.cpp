# include <learner.hpp>
# include <tagger.hpp>

using namespace Cnf;
using namespace SemiCnf;

int main(int argc, char **argv)
{
   Learner<Cnflearn> learner(*(argv+1), // template
         *(argv+2), // corpus
         1000000); // poolsize

   learner.init();
   learner.learn(50,1); // iter=50, L2-regularization
   learner.save(*(argv+3)); // save model parameter

   Tagger<Cnftagger> tagger(*(argv+1), // template
         1000000); // poolsize
   tagger.read(*(argv+3)); // read model parameter
   tagger.tagging(*(argv+2)); // tagging

   return 0;
}
