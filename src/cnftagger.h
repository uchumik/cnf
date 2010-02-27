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
# ifndef CNF_T_H
# define CNF_T_H

# include <cnflearn.h>
# include <myutil.h>

typedef struct viterbinode
{
   int id;
   float cost;
   viterbinode *join;
} vnode;

class Cnftagger
{
   public:
      /**
       * Construct Cnftagger
       * @param   tmpl template
       * @param   poolsize poolsize for dic
       */
      Cnftagger(const char *tmpl, unsigned int pool);
      ~Cnftagger();
      /**
       * Load Model
       * @param   model modelfile
       */
      void read(const char *model);
      /**
       * Tagging
       * @param   corpus   targetfile
       */
      void tagging(const char *corpus);
      /**
       * Viterbi
       * @param   Sequence target sequence
       * @param   cache cache
       * @param   labelids store predicted label ids
       */
      void viterbi(Sequence *s,
            AllocMemdiscard *cache,
            std::vector<int>& labelids);
      /**
       * Output
       * @param   Sequence target sequence
       * @param   labels   predicted label ids
       */
      void output(Sequence *s, std::vector<int>& labels);
      /**
       * Cache Size
       * @param   cache cachesize
       */
      void setcache(unsigned int cachesize);
      /**
       * Sequence Colsize
       * @param   sqcolsize   sqcolsize
       */
      void setsqcol(unsigned int sqcolsize);
      /**
       * Clear model
       */
      void clear();
      /// label index to surf
      std::vector<std::string> label2surf;
   private:
      Cnftagger();
      Cnftagger(const Cnftagger&);
      Cnftagger& operator=(const Cnftagger&);

      /// weight vector
      float *model;
      /// template file
      std::string tmpl;
      /// cache size for tagging
      unsigned int cachesize;
      /// labelsize
      unsigned int labelsize;
      /// unigram feature dic
      Dic *ufeatures;
      /// bigram feature dic
      Dic *bfeatures;
      /// Pool Allocator
      PoolAlloc *ac;
      /// first weight in template
      std::vector<int> fwit;
      /// sequence col size
      unsigned int sqcolsize;
      /// feature to template index
      //std::map<std::string,int> f2t;
      /// valid
      bool valid;
      /// unigram parameters
      int umid;
      bool bonly;
      int botmpl;

      /**
       * template validation
       */
      bool check(char *t);
      /**
       * for template validation
       */
      bool tmplcheck();
      /**
       * feature expand
       * @param tmpl template
       * @param s   sequence
       * @param current current row number
       */
      char* expand(char *tp, Sequence *s, int current);
      /**
       * store feature set
       */
      void storefset(Sequence *s, std::vector<feature_t>& featureset);
      /**
       * calc bigram cost
       */
      float getbcost(int bias, int label, feature_t *featureset);

      //void setf2t(FILE *fp);
      void setlabel(FILE *fp);
      void setfwit(FILE *fp);
      void setparams(FILE *fp);
      void setufeatures(FILE *fp);
      void setbfeatures(FILE *fp);

      void initlattice(vnode **lattice,
            std::vector<feature_t>& featureset);
};

# endif /* CNF_T_H */
