# include <tagger.hpp>
# include <cstdio>
# include <cstdlib>
# include <getopt.h>
# include <string>
# include <iostream>

using namespace Cnf;
using namespace SemiCnf;

/* Parameters */
static unsigned int block = 64;
static unsigned int pool = 1000000;
static unsigned int cache = 1024*100000;
static unsigned int sqcol = 3;
static std::string algorithm = "cnf";
static std::string tmpl;
static std::string corpus;
static std::string model;

void usage(int argc, char **argv)
{
   std::cerr << "Usage:" << argv[0] << " [options]" << std::endl;
   std::cerr << "-l, --learner\t" << "learning algorithm [cnf,semicnf](default cnf)" << std::endl;
   std::cerr << "-t, --template=FILE\t" << "template" << std::endl;
   std::cerr << "-c, --corpus=FILE\t" << "test corpus" << std::endl;
   std::cerr << "-m, --model=FILE\t" << "modelfile" << std::endl;
   std::cerr << "--block=INT\t" << "block size of pool allocator(default 64)" << std::endl;
   std::cerr << "--pool=INT\t" << "pool size of pool allocator(default 1000000)" << std::endl;
   std::cerr << "--cache=INT\t" << "cache size(default 1024*100000)" << std::endl;
   std::cerr << "--sqcol=INT\t" << "input sequence's col size(default 3)" << std::endl;
   exit(1);
}

int set(const char *pname, const char *optarg)
{
   if (std::strcmp(pname,"block") == 0)
   {
      block = (unsigned int)atoi(optarg);
   }
   else if (std::strcmp(pname,"pool") == 0)
   {
      pool = (unsigned int)atoi(optarg);
   }
   else if (std::strcmp(pname,"cache") == 0)
   {
      cache = (unsigned int)atoi(optarg);
   }
   else if (std::strcmp(pname,"sqcol") == 0)
   {
      sqcol = (unsigned int)atoi(optarg);
   }
   return 0;
}

int getparams(int argc, char **argv)
{
   int c;
   while (true)
   {
      static struct option long_options[] =
      {
         {"learner", required_argument, 0, 'l'},
         {"template", required_argument, 0, 't'},
         {"corpus", required_argument, 0, 'c'},
         {"model", required_argument, 0, 'm'},
         {"block", required_argument, 0, 0},
         {"pool", required_argument, 0, 0},
         {"cache", required_argument, 0, 0},
         {"sqcol", required_argument, 0, 0},
         {0, 0, 0, 0}
      };
      int option_index = 0;
      c = getopt_long (argc, argv, "l:t:c:m:", long_options, &option_index);

      if (c == -1)
         break;

      switch (c)
      {
         case 0:
            if (long_options[option_index].flag != 0)
               break;
            set(long_options[option_index].name, optarg);
            break;

         case 'l':
            algorithm = optarg;
            break;

         case 't':
            tmpl = optarg;
            break;

         case 'c':
            corpus = optarg;
            break;

         case 'm':
            model = optarg;
            break;

         case '?':
         default:
            usage(argc, argv);
      }
   }

   if (optind < argc)
   {
      std::cerr << "non-option ARGV-elements: ";
      while (optind < argc)
         std::cerr << argv[optind++] << ' ';
      std::cerr << std::endl;
      usage(argc,argv);
   }
}

int main(int argc, char **argv)
{
   if (argc < 7)
   {
      usage(argc, argv);
   }
   getparams(argc,argv);
   if (algorithm == "cnf")
   {
      Tagger<Cnftagger> tagger(tmpl.c_str(), pool);
      tagger.setcache(cache);
      tagger.setsqcol(sqcol);
      tagger.read(model.c_str());
      tagger.tagging(corpus.c_str());
   }
   else if (algorithm == "semicnf")
   {
      Tagger<SemiCnftagger> tagger(tmpl.c_str(), pool);
      tagger.setcache(cache);
      tagger.setsqcol(sqcol);
      tagger.read(model.c_str());
      tagger.tagging(corpus.c_str());
   }
   else
   {
      usage(argc,argv);
   }
   return 0;
}
