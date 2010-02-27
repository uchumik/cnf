/** 
 * Conditional Neural Fields with FOLOS
 * Copyright (C) 2010 - Kei Uchiumi
 *
 * Implementation of Conditional Neural Fields with FOLOS.
 * Jian Peng, Liefeng Bo and Jinbo Xu. "Conditional Neural Fields".
 * The 23rd Annual Conference on Neural Information Processing Systems (NIPS 2009)
 * John Duchi and Yoram Singer. "Online and Batch Learning using Forward Backward Splitting".
 * Journal of Machine Learning Research (JMLR 2009) and Neural Information Processing Systems (NIPS 2009).
 *
 */
# ifndef CNF_H
# define CNF_H

# include <cmath>
# include <map>
# include <set>
# include <string>
# include <vector>
# include <sequence.h>
# include <dic.h>
# include <allocpl.h>
# include <allocmd.h>
# include <myutil.h>
# include <sparsevect.h>


typedef struct
{
   std::vector<int> uf;
   std::vector<int> ut;
   std::vector<int> bf;
   std::vector<int> bt;
} feature_t;

typedef struct
{
   float _alpha;
   float _beta;
   float _lcost;
} fbnode;

class Cnflearn
{
   public:
      /**
       * Construct Cnflearn
       * @param   tmpl template
       * @param   corpus   corpus
       * @param   poolsize poolsize for dic
       */
      Cnflearn(const char *tmpl, const char *corpus, unsigned int poolsize);
      ~Cnflearn();

      /** Learn Parameters
       * @param   iter  iteration
       * @param   regularize  0:L1,1:L2
       */
      void learn(unsigned int iter, unsigned int reg);
      /** Save Model
       * @param   save  savefile
       */
      void save(const char *save);
      /**
       * Initialize
       */
      bool init();
      /**
       * Cache Size
       * @param   cache cachesize
       */
      void setcache(unsigned int cachesize);
      /**
       * Penalty
       * @param   aw alpha_w
       * @param   au alpha_u
       * @param   at alpha_t
       */
      void setpenalty(float w, float u, float t);
      /**
       * Feature Rejection
       * @param   bound bound 
       */
      void setbound(unsigned int bound);
      /**
       * Sequence Colsize
       * @param   sqcolsize   sqcolsize
       */
      void setsqcol(unsigned int sqcolsize);
      /**
       * Label Col
       * @param   labelcol labelcol
       */
      void setlabelcol(unsigned int labelcol);
      static inline double max (double x, double y)
      {
         return x > y ? x: y;
      }
      static inline double min (double x, double y)
      {
         return x > y ? y: x;
      }
      static inline int sign(float x)
      {
         int sign = (x > 0) - (x < 0);
         if (sign)
         {
            return sign;
         }
         else
         {
            return 1;
         }
      }
      static inline double myexp(double x)
      {
# define A0 (1.0)
# define A1 (0.125)
# define A2 (0.0078125)
# define A3 (0.00032552083)
# define A4 (1.0172526e-5)
         if (x < -13.0)
         {
            return 0;
         }
         bool reverse = false;
         if (x < 0)
         {
            x = -x;
            reverse = true;
         }
         double y;
         y = A0+x*(A1+x*(A2+x*(A3+x*A4)));
         y *= y;
         y *= y;
         y *= y;
         if (reverse)
         {
            y = 1./y;
         }
         return y;
# undef A0
# undef A1
# undef A2
# undef A3
# undef A4
      }
      static inline double logistic(double a)
      {
         return 1./(1+Cnflearn::myexp(-a));
      }
   private:
      Cnflearn();
      Cnflearn(const Cnflearn&);
      Cnflearn& operator=(const Cnflearn&);

      /// valid
      bool valid;
      /// The weight vector
      float *model;
      /// penalty cache
      float *pcache;
      /// penalty parameter
      float penalty;
      /// learning rate
      float eta;
      /// Lambda for detecting eta
      float lambda;
      /// coefficients for regularization
      float c[3];
      /// current coefficients
      float cc[3];
      /// template file
      std::string tmpl;
      /// training corpus
      std::string corpus;
      /// cache size for update
      unsigned int cachesize;
      /// bound of feature
      unsigned int bound;
      /// labelsize
      unsigned int labelsize;
      /// labels dic
      Dic *labels;
      /// feature dic
      Dic *features;
      /// unigram feature dic
      Dic *ufeatures;
      /// bigram feature dic
      Dic *bfeatures;
      /// Pool Allocator
      PoolAlloc *ac;
      /// weights of gate function
      unsigned int nk;
      /// weights of gate function
      std::vector<float> theta;
      /// first weight in template
      std::vector<int> fwit;
      /// sequence col size
      unsigned int sqcolsize;
      /// label col position in sequence
      unsigned int labelcol;
      /// instances size
      unsigned int instance;
      /// feature to template index
      //std::map<std::string,int> f2t;
      /// label index to surf
      std::vector<char*> label2surf;
      /// max id of unigram feature functions
      unsigned int umid;
      /// max id of bigram feature functions
      unsigned int bmid;
      /// parameters
      unsigned int parameters;
      /// correct tags
      unsigned int corrects;
      /// tags
      unsigned int tags;

      /**
       * report learning err
       */
      void lreport(unsigned int i);
      /**
       * report function
       */
      void report();
      /**
       * template validation function
       */
      bool check(char *t);
      /**
       * for template validation
       */
      bool tmplcheck();
      /**
       * label extraction
       * @param s Sequence
       */
      void extlabel(Sequence *s);
      /**
       * feature extraction
       */
      void extfeature();
      /**
       * feature extract
       */
      void extract(Sequence *s);
      /**
       * feature expand
       * @param tmpl template
       * @param s   sequence
       * @param current current row number
       */
      char* expand(char *tp, Sequence *s, int current);
      /**
       * feature rejection
       */
      void boundfeature();
      /**
       * store feature function
       */
      void storeff(nodeptr p, nodeptr nil);
      /**
       * init model and pcache
       */
      void initmodel();
      /**
       * decay eta and penalties
       */
      void decay(int t);
      /**
       * get correct lavbels
       */
      void getclabels(Sequence *s, std::vector<int>& labels);
      /**
       * store feature set
       */
      void storefset(Sequence *s, std::vector<feature_t>& featureset);
      /**
       * init lattice
       */
      void initlattice(fbnode **lattice, std::vector<feature_t>& featureset);
      /**
       * calc bigram cost
       */
      float getbcost(int bias, int label, feature_t *featureset);
      /**
       * forward
       */
      float forward(fbnode **lattice, int col, std::vector<feature_t>& featureset);
      /**
       * backward
       */
      float backward(fbnode **lattice, int col, std::vector<feature_t>& featureset);
      /**
       * update parameters
       */
      void update(Sequence *sq, AllocMemdiscard *cache, unsigned int reg);
      /**
       * calc correct vector
       */
      void getcorrectv(std::vector<int>& corrects,
            std::vector<feature_t>& featureset,
            SparseVector *v);
      /**
       * calc expected vector
       */
      void getgradient(fbnode **lattice,
            int col,
            std::vector<feature_t>& featureset,
            SparseVector *v,
            float z,
            std::vector<int>& corrects);
      void upbweight(int bias,
            int label,
            feature_t *bf,
            SparseVector *v,
            float expect);
      void upuweight(int label,
            feature_t *uf,
            SparseVector *v,
            float expect);
      void l1_regularize(SparseVector *v);
      void l2_regularize(SparseVector *v);
      void inversef(nodeptr p, nodeptr nil, std::vector<char*>& f);
      bool bonly;
      int botmpl;

      static inline double logsumexp(double x, double y, bool flg)
      {
         if (flg)
         {
            return y; // init mode
         }
         if (x == y)
         {
            return x + 0.69314718055; // log(2)
         }
         double vmin = Cnflearn::min(x,y);
         double vmax = Cnflearn::max(x,y);
         if (vmax > vmin + 50)
         {
            return vmax;
         }
         else
         {
            return vmax + std::log(Cnflearn::myexp(vmin-vmax)+1.0);
         }

      }
};
# endif /* CNF_H */
