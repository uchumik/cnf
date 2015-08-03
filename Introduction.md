# Introduction #

Cnflearn is a learner class and Cnftagger is a tagger class of CNF.
These class "Cnflearn" and "Cnftagger" use the similar formats as CRF++ for template files and data files.
Cnflearn is a online learner implementated with FOLOS.

# Usage #
```
$ hg clone https:/cnf.googlecode.com/hg/ cnf
$ cd cnf
$ make
$ ./src/learner
Usage:./src/learner [options]
-l, --learner	learning algorithm [cnf,semicnf](default cnf)
-t, --template=FILE	template
-c, --corpus=FILE	training corpus
-s, --save=FILE	modelfile
--l1	use L1-regularization(default)
--l2	use L2-regularization
--bound=INT	threshold of feature-frequency for CNF(default 3)
--fbound=INT	threshold of token feature-frequency for Semi-CNF(default 3)
--sbound=INT	threshold of segment feature-frequency for Semi-CNF(default 1)
--penalty=FLOATS	penalties for regularization
--lambda=FLOAT	parameter for detecting learning-rate eta (default 1.0)
--alpha=FLOAT	parameter for detecting learning-rate eta for Semi-CNF(default 1.1). must be (alpha > 1.0)
--block=INT	block size of pool allocator(default 64)
--pool=INT	pool size of pool allocator(default 1000000)
--cache=INT	cache size(default 1024*100000)
--iter=INT	iteration(default 50)
--sqcol=INT	input sequence's col size(default 3)
--sqarraysize=INT	input sequence's array size(default 1000)
--sqallocsize=INT	input sequence's alloc size(default 4096*1000)
--labelcol=INT	label col in input sequence(default 2)

$ ./src/tagger
Usage:./src/tagger [options]
-l, --learner	learning algorithm [cnf,semicnf](default cnf)
-t, --template=FILE	template
-c, --corpus=FILE	test corpus
-m, --model=FILE	modelfile
--block=INT	block size of pool allocator(default 64)
--pool=INT	pool size of pool allocator(default 1000000)
--cache=INT	cache size(default 1024*100000)
--sqcol=INT	input sequence's col size(default 3)
--sqarraysize=INT	input sequence's array size(default 1000)
--sqallocsize=INT	input sequence's alloc size(default 4096*1000)
```

## Running CNF on the CoNLL chunking task ##
### cnf ###
```
$ ./src/learner -t src/template -c data/conll2000/train.txt -s cnf.save --l2
$ ./src/tagger -t src/template -c data/conll2000/test.txt -m cnf.save | ./src/conlleval 
 
processed 47377 tokens with 23852 phrases; found: 23718 phrases; correct: 22293.
accuracy:  96.02%; precision:  93.99%; recall:  93.46%; FB1:  93.73
             ADJP: precision:  81.08%; recall:  75.34%; FB1:  78.11  407
             ADVP: precision:  83.71%; recall:  80.72%; FB1:  82.19  835
            CONJP: precision:  55.56%; recall:  55.56%; FB1:  55.56  9
             INTJ: precision: 100.00%; recall:  50.00%; FB1:  66.67  1
              LST: precision:   0.00%; recall:   0.00%; FB1:   0.00  0
               NP: precision:  94.39%; recall:  93.88%; FB1:  94.14  12355
               PP: precision:  96.81%; recall:  97.78%; FB1:  97.29  4859
              PRT: precision:  83.15%; recall:  69.81%; FB1:  75.90  89
             SBAR: precision:  87.72%; recall:  85.42%; FB1:  86.55  521
               VP: precision:  93.95%; recall:  93.62%; FB1:  93.78  4642
```

### markov/semi-markov cnf ###
```
$ ./src/learner --learner semicnf -c data/conll2000/train.txt -t src/semitemplate -s semi.save --pool=13000000 --block=32 --penalty=0.001,0.001,0.001,0.001,0.001 --sbound=1 --l2
$ ./src/tagger --learner=semicnf -t src/semitemplate -c data/conll2000/test.txt -m semi.save --pool=10000000 --block=32 | ./src/conlleval

processed 47377 tokens with 23852 phrases; found: 23846 phrases; correct: 22333.
accuracy:  95.91%; precision:  93.66%; recall:  93.63%; FB1:  93.64
             ADJP: precision:  78.12%; recall:  74.20%; FB1:  76.11  416
             ADVP: precision:  82.41%; recall:  80.60%; FB1:  81.49  847
            CONJP: precision:  55.56%; recall:  55.56%; FB1:  55.56  9
             INTJ: precision: 100.00%; recall: 100.00%; FB1: 100.00  2
              LST: precision:   0.00%; recall:   0.00%; FB1:   0.00  0
               NP: precision:  94.10%; recall:  94.09%; FB1:  94.09  12421
               PP: precision:  96.83%; recall:  97.90%; FB1:  97.36  4864
              PRT: precision:  76.32%; recall:  82.08%; FB1:  79.09  114
             SBAR: precision:  86.41%; recall:  83.18%; FB1:  84.76  515
               VP: precision:  93.88%; recall:  93.88%; FB1:  93.88  4658
```

# Sample Code #
## sample.cpp ##
```
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
```

## compile ##
```
$ cd src
$ ln -s libcnf.so.0.0.1 libcnf.so
$ g++ sample.cpp -I. -I../lib -L. -lcnf
$ ./a.out template ../data/conll2000/train.txt test.save
```

<a href='Hidden comment: 
== cnflearn_main.cpp ==
```
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
```

== cnflearn_tagger.cpp ==
```
# include <cnftagger.h>
# include <cstdio>

using namespace Cnf;
int main(int argc, char **argv)
{
        Cnftagger cnf(*(argv+1),500000);
        cnf.read(*(argv+2));
        cnf.tagging(*(argv+3));
}
```

== semicnflearn_main.cpp ==
```
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
```

== semicnftagger_main.cpp ==
```
# include <semicnftagger.h>
# include <cstdio>

using namespace SemiCnf;
int main(int argc, char **argv)
{
   SemiCnftagger tagger(*(argv+1), 10000000);
   tagger.read(*(argv+2));
   tagger.tagging(*(argv+3));
   return 0;
}
```
'></a>


## template for cnf ##
```
# Unigram
U00:%x[-2,0,0]
U01:%x[-1,0,0]
U02:%x[0,0,0]
U03:%x[1,0,0]
U04:%x[2,0,0]
U05:%x[-1,0,0]/%x[0,0,0]
U06:%x[0,0,0]/%x[1,0,0]

U10:%x[-2,1,0]
U11:%x[-1,1,0]
U12:%x[0,1,0]
U13:%x[1,1,0]
U14:%x[2,1,0]
U15:%x[-2,1,0]/%x[-1,1,0]
U16:%x[-1,1,0]/%x[0,1,0]
U17:%x[0,1,0]/%x[1,1,0]
U18:%x[1,1,0]/%x[2,1,0]

U20:%x[-2,1,0]/%x[-1,1,0]/%x[0,1,0]
U21:%x[-1,1,0]/%x[0,1,0]/%x[1,1,0]
U22:%x[0,1,0]/%x[1,1,0]/%x[2,1,0]

# Bigram
B
```

## template for semi-markov cnf ##
```
# Unigram Segment
S01:%s[15,0,0]
S02:%s[15,1,0]

# Bigram Segment
T

# Unigram
U00:%x[-2,0,0]
U01:%x[-1,0,0]
U02:%x[0,0,0]
U03:%x[1,0,0]
U04:%x[2,0,0]
U05:%x[-1,0,0]/%x[0,0,0]
U06:%x[0,0,0]/%x[1,0,0]

U10:%x[-2,1,0]
U11:%x[-1,1,0]
U12:%x[0,1,0]
U13:%x[1,1,0]
U14:%x[2,1,0]
U15:%x[-2,1,0]/%x[-1,1,0]
U16:%x[-1,1,0]/%x[0,1,0]
U17:%x[0,1,0]/%x[1,1,0]
U18:%x[1,1,0]/%x[2,1,0]

U20:%x[-2,1,0]/%x[-1,1,0]/%x[0,1,0]
U21:%x[-1,1,0]/%x[0,1,0]/%x[1,1,0]
U22:%x[0,1,0]/%x[1,1,0]/%x[2,1,0]

# Bigram
B
```
### format ###
  * token feature template
```
# Unigram token feature
# First character 'U' is a specified character to identify a template-type.
# 01 is a prefix to prevent feature-collisions that extracted from different kinds of templates.
# %x[row,column,weight]
# row is a relative location from current position. 
# column is a location in the line of tokens of input-sequence.
# weight is a initial parameter for gate-function.
U01:%x[0,0,0]

# Bigram token feature
# Bigram template generates feature functions f(y_{i-1},y_{i},x_{i}).
# If you want to only use label-transition, you just write 'B'.
B01:%x[0,0,0]
B
```
  * segment feature template
```
# Unigram segment feature
# %s[length,column,weight]
# length is a segment-length that extract from input-sequence.
# length has to be longer than all of correct segments in a training-corpus.
S01:%x[15,0,0]

# Bigram segment feature
# Bigram segment template generates feature functions g(y_{i-1},y_{i},s_{i}).
# If you want to only use label-transition of segments, you just write 'T'.
T01:%x[15,0,0]
T
```
### notices ###
  * Semi-markov CNF only accepts corpus in the IOB2 format.
> B for BEGIN, I for INSIDE, and O for OUTSIDE

> If you want to use some kind of label, you have to write "B-label", "I-label".
  * If you want to use some segment-templates that length different from each other, you have to arrange them in descending order of length.