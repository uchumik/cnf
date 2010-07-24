# include <cnftagger.h>
# include <cstdio>

using namespace Cnf;
int main(int argc, char **argv)
{
	Cnftagger cnf(*(argv+1),500000);
	cnf.read(*(argv+2));
	cnf.tagging(*(argv+3));
}
