/**
 * A Hybrid Markov/Semi-markov Conditional Neural Fields
 * written by Kei Uchiumi
 * Copyright (C) 2010 - Kei Uchiumi
 */
# ifndef SEMI_CNF_T_H
# define SEMI_CNF_T_H

# include <semicnflearn.h>
# include <myutil.h>

typedef struct vsegment
{
   int sl;  // segment label
   int bl;  // begin of local label
   int il;  // inside of local label
   int uid; // unigram segment id
   int bid; // bigram segment id
   int ut;  // unigram segment template id
   int bt;  // bigram segment template id
   int len; // length
   float _lcost;
   vsegment *join;
} vsegment_t;

typedef struct
{
   std::vector<vsegment_t*> prev;
   std::vector<vsegment_t*> next;
   feature_t tokenf;
} vnode_t;

class SemiCnftagger
{
   public:
      /**
       * @param tmpl template-file
       * @param pool poolsize
       */
      SemiCnftagger(const char *tmpl, unsigned int pool);
      virtual ~SemiCnftagger();
      /**
       * @param model model-file
       */
      void read(const char *model);
      /**
       * @param corpus target-file
       */
      void tagging(const char *corpus);
      /**
       * @param Sequence target-sequence
       * @param cache cache
       * @param labelids vector to store predicted label ids
       */
      void viterbi(Sequence *s, AllocMemdiscard *cache, std::vector<int>& labelids);
      /**
       * @param Sequence target sequence
       * @param labels predicted label ids
       */
      void output(Sequence *s, std::vector<int>& labels);
      /**
       * @param cache cachesize
       */
      void setcache(unsigned int cachesize);
      /**
       * Sequence colsize
       * @param sqcolsize sqcolsize
       */
      void setsqcol(unsigned int sqcolsize);
      /// label index to surface
      std::vector<std::string> label2surf;
   private:
      SemiCnftagger();
      SemiCnftagger(const SemiCnftagger&);
      SemiCnftagger& operator=(const SemiCnftagger&);

      /// valid
      bool valid;
      /// weight vector
      float *model;
      /// template file
      std::string tmpl;
      /// cache size for tagging
      unsigned int cachesize;
      /// local labelsize
      unsigned int llabelsize;
      /// segment labelsize
      unsigned int slabelsize;
      /// unigram feature dic
      Dic *ufeatures;
      /// bigram feature dic
      Dic *bfeatures;
      /// unigram segment dic
      Dic *usegments;
      /// bigram segment dic
      Dic *bsegments;
      /// Pool Allocator
      PoolAlloc *ac;
      /// first weight in template
      std::vector<int> fwit;
      /// sequence col size
      unsigned int sqcolsize;
      /// max id of unigram feature function
      unsigned int umid;
      /// max id of bigram feature function
      unsigned int bmid;
      /// max id of unigram segment feature
      unsigned int usid;
      /// max id of bigram segment feature
      unsigned int bsid;
      /// mapping segment label to local labels
      void mapping(char *l, int id);

      /**
       * template validation
       */
      bool check(std::string& t);
      /**
       * for template validation
       */
      bool tmplcheck();
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
       * store feature set
       */
      void storefset(Sequence *s, std::vector<vnode_t>& lattice, AllocMemdiscard *cache);
      /**
       * init lattice
       */
      void initlattice(std::vector<vnode_t>& lattice);
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

      void readparamsize(std::string& line);
      void readllabelsize(std::string& line);
      void readslabelsize(std::string& line);
      void readllabels(std::ifstream& in);
      void readmap(std::ifstream& in);
      void readparams(std::ifstream& in);
      void readfwit(std::ifstream& in);
      void readufeatures(std::ifstream& in);
      void readbfeatures(std::ifstream& in);
      void readusegments(std::ifstream& in);
      void readbsegments(std::ifstream& in);

      /// segment label to local labels
      std::map<int, llabel_t> sl2ll;
      bool bonly;
      bool tonly;
      int botmpl;
      int totmpl;
      int tmpli;
      int smaxlen;
      int slen;
};
# endif /* SEMI_CNF_T_H */
