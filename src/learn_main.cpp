# include <learner.hpp>
# include <cstdio>
# include <cstdlib>
# include <getopt.h>
# include <string>
# include <iostream>

using namespace Cnf;
using namespace SemiCnf;

/* Parameters */
static int regularize = 0;
static int iter = 50;
static unsigned int block = 64;
static unsigned int pool = 1000000;
static unsigned int bound = 3;
static unsigned int fbound = 3;
static unsigned int sbound = 1;
static unsigned int sqcol = 3;
static unsigned int sqallocsize = 4096*1000;
static unsigned int sqarraysize = 1000;
static unsigned int labelcol = 2;
static unsigned int cache = 1024*100000;
static float cnfpenalty[3] = {0.0001,0.0001,0.0001};
static float semicnfpenalty[5] = {0.0001,0.0001,0.0001,0.0001,0.0001};
static float lambda = 1.0;
static float alpha = 1.1;
static std::string algorithm = "cnf";
static std::string tmpl;
static std::string corpus;
static std::string save;

void usage(int argc, char **argv)
{
   std::cerr << "Usage:" << argv[0] << " [options]" << std::endl;
   std::cerr << "-l, --learner\t" << "learning algorithm [cnf,semicnf](default cnf)" << std::endl;
   std::cerr << "-t, --template=FILE\t" << "template" << std::endl;
   std::cerr << "-c, --corpus=FILE\t" << "training corpus" << std::endl;
   std::cerr << "-s, --save=FILE\t" << "modelfile" << std::endl;
   std::cerr << "--l1\t" << "use L1-regularization(default)" << std::endl;
   std::cerr << "--l2\t" << "use L2-regularization" << std::endl;
   std::cerr << "--bound=INT\t" << "threshold of feature-frequency for CNF(default 3)" << std::endl;
   std::cerr << "--fbound=INT\t" << "threshold of token feature-frequency for Semi-CNF(default 3)" << std::endl;
   std::cerr << "--sbound=INT\t" << "threshold of segment feature-frequency for Semi-CNF(default 1)" << std::endl;
   std::cerr << "--penalty=FLOATS\t" << "penalties for regularization" << std::endl;
   std::cerr << "--lambda=FLOAT\t" << "parameter for detecting learning-rate eta (default 1.0)" << std::endl;
   std::cerr << "--alpha=FLOAT\t" << "parameter for detecting learning-rate eta for Semi-CNF(default 1.1). must be (alpha > 1.0)" << std::endl;
   std::cerr << "--block=INT\t" << "block size of pool allocator(default 64)" << std::endl;
   std::cerr << "--pool=INT\t" << "pool size of pool allocator(default 1000000)" << std::endl;
   std::cerr << "--cache=INT\t" << "cache size(default 1024*100000)" << std::endl;
   std::cerr << "--iter=INT\t" << "iteration(default 50)" << std::endl;
   std::cerr << "--sqcol=INT\t" << "input sequence's col size(default 3)" << std::endl;
   std::cerr << "--sqarraysize=INT\t" << "input sequence's array size(default 1000)" << std::endl;
   std::cerr << "--sqallocsize=INT\t" << "input sequence's alloc size(default 4096*1000)" << std::endl;
   std::cerr << "--labelcol=INT\t" << "label col in input sequence(default 2)" << std::endl;

   exit(1);
}

int set(const char *pname, const char *optarg)
{
   if (std::strcmp(pname,"bound") == 0)
   {
      bound = atoi(optarg);
   }
   else if (std::strcmp(pname,"fbound") == 0)
   {
      fbound = atoi(optarg);
   }
   else if (std::strcmp(pname,"sbound") == 0)
   {
      sbound = atoi(optarg);
   }
   else if (std::strcmp(pname,"penalty") == 0)
   {
      int c = 0;
      float pena[5];
      const char *h = optarg;
      for (const char *p = h; *p != '\0'; ++p)
      {
         if (*p == ',')
         {
            pena[c] = atof(h);
            h = p+1;
            ++c;
         }
      }
      if (*h != '\0')
      {
         pena[c] = atof(h);
         ++c;
      }
      if (c == 3)
      {
         for (int i = 0; i < c; ++i)
         {
            cnfpenalty[i] = pena[i];
         }
      }
      else if (c == 5)
      {
         for (int i = 0; i < c; ++i)
         {
            semicnfpenalty[i] = pena[i];
         }
      }
      else
      {
         std::cerr << "ERR: found unknown penalty parameter " << optarg << std::endl;
         std::cerr << "parameter format follow the next" << std::endl;
         std::cerr << "cnf: penalty=0.0001,0.0001,0.0001" << std::endl;
         std::cerr << "semicnf: penalty=0.0001,0.0001,0.0001,0.0001,0.0001" << std::endl;
         exit(1);
      }
   }
   else if (std::strcmp(pname,"lambda") == 0)
   {
      lambda = atof(optarg);
   }
   else if (std::strcmp(pname,"alpha") == 0)
   {
      alpha = atof(optarg);
   }
   else if (std::strcmp(pname,"block") == 0)
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
   else if (std::strcmp(pname,"iter") == 0)
   {
      iter = atoi(optarg);
   }
   else if (std::strcmp(pname,"sqcol") == 0)
   {
      sqcol = (unsigned int)atoi(optarg);
   }
   else if (std::strcmp(pname,"sqarraysize") == 0)
   {
      sqarraysize = (unsigned int)atoi(optarg);
   }
   else if (std::strcmp(pname,"sqallocsize") == 0)
   {
      sqallocsize = (unsigned int)atoi(optarg);
   }
   else if (std::strcmp(pname,"labelcol") == 0)
   {
      labelcol = (unsigned int)atoi(optarg);
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
         /* These options set a flag. */
         {"l1",   no_argument,   &regularize, 0},
         {"l2",   no_argument,   &regularize, 1},
         /* These options don't set a flag.
            We distinguish them by their indices. */
         {"learner", required_argument,   0, 'l'},
         {"template",   required_argument,   0, 't'},
         {"corpus",  required_argument,   0, 'c'},
         {"save", required_argument,   0, 's'},
         {"bound",   required_argument,   0, 0},
         {"fbound",  required_argument,   0, 0},
         {"sbound",  required_argument,   0, 0},
         {"penalty", required_argument,   0, 0},
         {"lambda",  required_argument,   0, 0},
         {"alpha",   required_argument,   0, 0},
         {"block",   required_argument,   0, 0},
         {"pool", required_argument,   0, 0},
         {"cache", required_argument,   0, 0},
         {"iter", required_argument,   0, 0},
         {"sqcol", required_argument,   0, 0},
         {"sqarraysize", required_argument,   0, 0},
         {"sqallocsize", required_argument,   0, 0},
         {"labelcol", required_argument,   0, 0},
         {0, 0, 0, 0}
      };
      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long (argc, argv, "l:t:c:s:",
            long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
         break;

      switch (c)
      {
         case 0:
            /* If this option set a flag, do nothing else now. */
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

         case 's':
            save = optarg;
            break;

         case '?':
            /* getopt_long already printed an error message. */
            usage(argc,argv);
            break;

         default:
            usage(argc,argv);
      }
   }

   /* Print any remaining command line arguments (not options). */
   if (optind < argc)
   {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
         printf ("%s ", argv[optind++]);
      putchar ('\n');
      usage(argc,argv);
   }

   return 0;
}

bool check()
{
   if (tmpl == "" ||
         corpus == "" ||
         save == "" ||
         sqcol <= 0 ||
         lambda <= 0 || 
         iter <= 0)
   {
      return false;
   }
   return true;
}

int main (int argc, char **argv)
{
   if (argc < 7)
   {
      usage(argc,argv);
   }
   getparams(argc,argv);
   if (!check())
   {
      usage(argc,argv);
   }
   if (algorithm == "cnf")
   {
      Learner<Cnflearn> learner(tmpl.c_str(), corpus.c_str(), pool);
      learner.setcache(cache);
      learner.setbound(bound);
      learner.setpenalty(cnfpenalty[0],cnfpenalty[1],cnfpenalty[2]);
      learner.setsqcol(sqcol);
      learner.setsqallocsize(sqallocsize);
      learner.setsqarraysize(sqarraysize);
      learner.setlabelcol(labelcol);
      learner.setlambda(lambda);
      learner.init();
      learner.learn((unsigned int)iter,regularize);
      learner.save(save.c_str());
   }
   else if (algorithm == "semicnf")
   {
      Learner<SemiCnflearn> learner(tmpl.c_str(), corpus.c_str(), pool);
      learner.setcache(cache);
      learner.setfbound(fbound);
      learner.setsbound(sbound);
      learner.setpenalty(semicnfpenalty[0],semicnfpenalty[1],semicnfpenalty[2],semicnfpenalty[3],semicnfpenalty[4]);
      learner.setsqcol(sqcol);
      learner.setsqallocsize(sqallocsize);
      learner.setsqarraysize(sqarraysize);
      learner.setlabelcol(labelcol);
      learner.setlambda(lambda);
      learner.setalpha(alpha);
      learner.init();
      learner.learn((unsigned int)iter,regularize);
      learner.save(save.c_str());
   }
   else
   {
      usage(argc,argv);
   }
   return 0;
}
