# include "rtcrflearn.hpp"
# include "rtcrftagger.hpp"

using namespace RtCrf;
using namespace std;

int main(int argc, char **argv)
{
   Crflearn learner(*(argv+1),*(argv+2),1000000);
   learner.init();

   learner.learn(5,0);
   learner.save("test.model");

   Crftagger tagger(*(argv+1),1000000);
   tagger.read("test.model");
   tagger.tagging(*(argv+2));
   return 0;
}
