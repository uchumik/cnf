# ifndef __RTCRFLEARN__
# define __RTCRFLEARN__

# include "rtcrf.hpp"
# include "sparsevect.h"

namespace RtCrf
{
   typedef struct
   {
      float _alpha;
      float _beta;
      float _lcost;
   } fbnode;

   class Crflearn : public RtCrf::Crf
   {
      public:
         Crflearn(const char *tmpl, const char *corpus, unsigned int poolsize);
         virtual ~Crflearn();
         bool init();
         void learn(unsigned int iter, unsigned int reg);
         void save(const char *save);

         /// hyperparameters
         void setpenalty(float p);
         void setbound(unsigned int bound);
         void setlabelcol(unsigned int labelcol);
         void setlambda(float lambda);
      protected:
         //std::string delimit;
         std::string corpus;
         float *pcache;
         float penalty;
         float eta;
         float lambda;
         float c;
         float cc;
         unsigned int bound;
         unsigned int labelcol;
         unsigned int instance;
         std::vector<char*> label2surf;
         unsigned int parameters;
         unsigned int corrects;
         unsigned int tags;

         /// feature extraction
         void extfeature();
         void finsert(fsetv_t& fv);

         /// storing label and mapping id to surface
         void extlabel(sequential::Sequence *s);

         /// feature rejection
         void boundfeature();
         void storeff(nodeptr p, nodeptr nil);

         /// report
         void report();
         void initmodel();

         /// training
         void decay(int t);
         void lreport(unsigned int iter);
         void initlattice(fbnode **lattice, std::vector<feature_t>& featureset);
         float forward(fbnode **lattice, int col, std::vector<feature_t>& featureset);
         float backward(fbnode **lattice, int col, std::vector<feature_t>& featureset);
         void getclabels(sequential::Sequence *s, std::vector<int>& labels);
         //float getbcost(int bias, int label, feature_t *featureset);
         void update(sequential::Sequence *sq, AllocMemdiscard *cache, unsigned int reg);
         void getcorrectv(std::vector<int>& corrects,
               std::vector<feature_t>& featureset,
               SparseVector *v);
         void upbweight(int bias,
               int label,
               feature_t *bf,
               SparseVector *v,
               float expect);
         void upuweight(int label,
               feature_t *uf,
               SparseVector *v,
               float expect);
         void getgradient(fbnode **lattice,
               int col,
               std::vector<feature_t>& featureset,
               SparseVector *v,
               float z,
               std::vector<int>& corrects);
         void l1_regularize(SparseVector *v);
         void l2_regularize(SparseVector *v);

         /// save
         void inversef(nodeptr p, nodeptr nil, std::vector<char*>& f);
      private:
         Crflearn();
         Crflearn(const Crflearn&);
         Crflearn& operator=(const Crflearn&);
   };
}
# endif /* __RTCRFLEARN__ */
