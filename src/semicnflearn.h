/**
 * A Hybrid Markov/Semi-markov Conditional Neural Fields
 * written by Kei Uchiumi
 * Copyright (C) 2010 - Kei Uchiumi
 */
# ifndef SEMI_CNF_L_H
# define SEMI_CNF_L_H

# include <cmath>
# include <vector>
# include <map>
# include <sequence.h>
# include <dic.h>
# include <allocpl.h>
# include <allocmd.h>
# include <myutil.h>
# include <sparsevect.h>

typedef struct
{
   int sl;
   int bl;
   int il;
   int len;
} correct_t;

typedef struct
{
   int bl;
   int il;
} llabel_t;

typedef struct
{
   std::vector<int> uf;
   std::vector<int> ut;
   std::vector<int> bf;
   std::vector<int> bt;
} feature_t;

typedef struct
{
   int sl;  // segment label
   int bl;  // begin of local label
   int il;  // inside of local label
   int uid; // unigram segment id
   int bid; // bigram segment id
   int ut;  // unigram segment template id
   int bt;  // bigram segment template id
   int len; // length
   float _alpha;
   float _beta;
   float _lcost;
} segment_t;

typedef struct
{
   std::vector<segment_t*> prev;
   std::vector<segment_t*> next;
   feature_t tokenf;
} node_t;

class SemiCnflearn
{
   public:
      SemiCnflearn(const char *tmpl, const char *corpus, unsigned int poolsize);
      virtual ~SemiCnflearn();
      void learn(unsigned int iter, unsigned int reg);
      void save(const char *save);
      bool init();
      void setcache(unsigned int cachesize);
      void setpenalty(float bs, float us, float bf, float uf, float t);
      void setfbound(unsigned int fbound);
      void setsbound(unsigned int sbound);
      void setlabelcol(unsigned int labelcol);
      static inline double max(double x, double y)
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
      }
# undef A0
# undef A1
# undef A2
# undef A3
# undef A4
      static inline double logistic(double a)
      {
         return 1./(1+SemiCnflearn::myexp(-a));
      }
   private:
      SemiCnflearn();
      SemiCnflearn(const SemiCnflearn&);
      SemiCnflearn& operator=(const SemiCnflearn&);

      /// valid
      bool valid;
      /// weight vector
      float *model;
      /// penalty cache
      float *pcache;
      /// penalty parameter
      float penalty;
      /// learning rate
      float eta;
      /// Lambda for detecting eta
      float lambda;
      /// Alpha for detecting eta
      float alpha;
      /// coefficients for regularization
      float c[5];
      /// current coefficients
      float cc;
      /// template file
      std::string tmpl;
      /// training corpus
      std::string corpus;
      /// cache size for update
      unsigned int cachesize;
      /// bound of feature
      unsigned int fbound;
      /// bound of segment
      unsigned int sbound;
      /// local labelsize
      unsigned int llabelsize;
      /// segment labelsize
      unsigned int slabelsize;
      /// segment labels dic
      Dic *slabels;
      /// labels dic
      Dic *labels;
      /// feature dic
      Dic *features;
      /// unigram feature dic
      Dic *ufeatures;
      /// bigram feature dic
      Dic *bfeatures;
      /// segment dic
      Dic *segments;
      /// unigram segment dic
      Dic *usegments;
      /// bigram segment dic
      Dic *bsegments;
      /// Pool Allocator
      PoolAlloc *ac;
      /// weights of gate function
      unsigned int nk;
      std::vector<float> theta;
      /// first weight in template
      std::vector<int> fwit;
      /// sequence col size
      unsigned int sqcolsize;
      /// label col position in sequence
      unsigned int labelcol;
      /// instance size
      unsigned int instance;
      /// label index to surf
      std::vector<char*> label2surf;
      /// segment label to local labels
      std::map<int, llabel_t> sl2ll;
      /// max id of unigram feature functions
      unsigned int umid;
      /// max id of bigram feature functions
      unsigned int bmid;
      /// max id of unigram segment features
      unsigned int usid;
      /// max id of bigram segment features
      unsigned int bsid;
      /// parameters
      unsigned int parameters;

      /// mapping segment label to local labels
      void mapping(char *l, int id);

      /// report
      void lreport(unsigned int i);
      /**
       * report function
       */
      void report();
      /**
       * template validation function
       */
      bool check(std::string& t);
      /**
       * for template validation
       */
      bool tmplcheck();
      /**
       * label extraction
       * @param s Sequence
       */
      void extlabel(Sequence *s);
      char* cutbi(char *l);
      /**
       * feature extract
       */
      void extfeature();
      void extract(Sequence *s);
      /**
       * feature expand
       * @param t template
       * @param s Sequence
       * @param current current position
       */
      char* fexpand(std::string& t, Sequence *s, int current);
      /**
       * segment expand
       * @param t template
       * @param s Sequence
       * @param current current position
       * @param segments segments
       */
      int sexpand(std::string& t, Sequence *s, int current, std::vector<char*>& segments);
      /**
       * feature rejection
       */
      void boundfeature();
      /**
       * segment rejection
       */
      void boundsegment();
      /**
       * store feature function
       */
      void storeff(nodeptr p, nodeptr nil);
      /**
       * store segment feature function
       */
      void storesf(nodeptr p, nodeptr nil);
      /**
       * init model and pcache
       */
      void initmodel();
      /**
       * decay eta and penalties
       */
      void decay(int t);
      /**
       * update
       */
      void update(Sequence *s, AllocMemdiscard *cache, unsigned int reg);
      /**
       * store feature set
       */
      void storefset(Sequence *s, std::vector<node_t>& lattice, AllocMemdiscard *cache);
      /**
       * init lattice
       */
      void initlattice(std::vector<node_t>& lattice);
      /**
       * forward
       */
      float forward(std::vector<node_t>& lattice);
      /**
       *
       */
      float backward(std::vector<node_t>& lattice);
      /**
       *
       */
      int getbfbias(bool bos, bool eos, int label);
      int getbsid(bool bos, bool eos, int bid, int label);

      /**
       * get feature weight
       */
      float getfw(int id, int tmpl);
      /**
       * bigram segment cost
       * getbscost
       */
      float getbscost(int id, int tmpl, int label);
      /**
       * bigram feature cost
       * getbfcost
       */
      float getbfcost(feature_t *f, int bias, int label);
      /**
       * get correct labels
       */
      void getclabels(Sequence *sq, std::vector<correct_t>& corrects);
      /**
       * get correct vector
       */
      void getcorrectv(std::vector<correct_t>& corrects, std::vector<node_t>& lattice, SparseVector *v);
      /**
       * update unigram feature weight
       */
      void upufweight(int label, feature_t *uf, SparseVector *v, float expect);
      /**
       * update bigram feature weight
       */
      void upbfweight(int bias, int label, feature_t *bf, SparseVector *v, float expect);
      /**
       * update unigram segment feature weight
       */
      void upusweight(int label, int usid, int tmpl, SparseVector *v, float expect);
      /**
       * update bigram segment feature weight
       */
      void upbsweight(int id, int label, int tmpl, SparseVector *v, float expect);

      void getgradient(std::vector<node_t>& lattice, SparseVector *v, float z);
      void l1regularize(SparseVector *v);
      void l2regularize(SparseVector *v);
      void dumpllabels(std::ofstream &out);
      void dumpslabels(std::ofstream &out);
      void dumpfwit(std::ofstream &out);
      void dumpsl2ll(std::ofstream &out);
      void dumpparams(std::ofstream &out);
      void dumpfeatures(std::ofstream &out);
      void dumpufeatures(std::ofstream &out, std::vector<char*>& f);
      void dumpbfeatures(std::ofstream &out, std::vector<char*>& f);
      void dumpusegments(std::ofstream &out, std::vector<char*>& f);
      void dumpbsegments(std::ofstream &out, std::vector<char*>& f);
      void inversef(nodeptr p, nodeptr nil, std::vector<char*>& f);
      void callinv(nodeptr *p, nodeptr nil, std::vector<char*>& f);

      bool bonly;
      bool tonly;
      int botmpl;
      int totmpl;
      int tmpli;
      int smaxlen;
      int slen;
      float err;

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
         double vmin = SemiCnflearn::min(x,y);
         double vmax = SemiCnflearn::max(x,y);
         if (vmax > vmin + 50)
         {
            return vmax;
         }
         else
         {
            return vmax + std::log(SemiCnflearn::myexp(vmin-vmax)+1.0);
         }
      }

};
# endif /* SEMI_CNF_L_H */
