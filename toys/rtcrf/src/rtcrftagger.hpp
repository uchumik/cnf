# ifndef __RTCRFTAGGER__
# define __RTCRFTAGGER__

# include "rtcrf.hpp"

namespace RtCrf
{
   typedef struct viterbinode
   {
      int id;
      float cost;
      viterbinode *join;
   } vnode;

   class Crftagger : public RtCrf::Crf
   {
      public:
         Crftagger(const char *tmpl, unsigned int pool);
         virtual ~Crftagger();
         void read(const char *model);
         void tagging(const char *corpus);
         void viterbi(sequential::Sequence *s,
               AllocMemdiscard *cache,
               std::vector<int>& labelids);
         void output(sequential::Sequence *s, std::vector<int>& labels);
         void clear();
         std::vector<std::string> label2surf;
      protected:
         /// read model
         void setlabel(FILE *fp);
         void setparams(FILE *fp);
         void setufeatures(FILE *fp);
         void setbfeatures(FILE *fp);
         void initlattice(vnode **lattice,
               std::vector<feature_t>& featureset);
         //float getbcost(int bias, int label, feature_t *featureset);
      private:
         Crftagger();
         Crftagger(const Crftagger&);
         Crftagger& operator=(const Crftagger&);
   };
}
# endif /* __RTCRFTAGGER__ */
