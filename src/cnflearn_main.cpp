# include <cnflearn.h>
# include <cstdio>

using namespace Cnf;
int main(int argc, char **argv)
{
   Cnflearn cnf(*(argv+1), *(argv+2), 1000000);
   cnf.setpenalty(0.0001,0.0001,0.0001);
   cnf.init();
   cnf.learn(50,1);
   cnf.save(*(argv+3));
   return 0;
}
