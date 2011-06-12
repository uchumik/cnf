# ifndef __RTCRF__
# define __RTCRF__

# include "featuretemplate.hpp"
# include "sequence.h"
# include "allocmd.h"
# include "dic.h"
# include "myutil.h"
# include <cmath>
# include <utility>

namespace RtCrf
{
   typedef struct
   {
      std::vector<std::pair<int,double> > uf;
      std::vector<std::pair<int,double> > bf;
   } feature_t;

   class Crf
   {
      public:
         Crf(const char *tmpl,unsigned int poolsize);
         virtual ~Crf();
         unsigned int checkcolsize(const char *file,const char *delim);
         void setdelimit(const char *d);
         void setcache(unsigned int cachesize);
         void setsqarraysize(unsigned int sqarraysize);
         void setsqallocsize(unsigned int sqallocsize);
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
            double vmin = Crf::min(x,y);
            double vmax = Crf::max(x,y);
            if (vmax > vmin + 50)
            {
               return vmax;
            }
            else
            {
               return vmax + std::log(Crf::myexp(vmin-vmax)+1.0);
            }
         }
      protected:
         bool valid;
         float *model;
         std::string tmpl;
         std::string delimit;
         unsigned int cachesize;
         unsigned int labelsize;
         unsigned int sqcolsize;
         unsigned int sqarraysize;
         unsigned int sqallocsize;
         unsigned int umid;
         unsigned int bmid;
         unsigned int parameters;
         bool bonly;
         Dic *labels;
         Dic *features;
         Dic *ufeatures;
         Dic *bfeatures;
         PoolAlloc *ac;
         ftemplate *ft;

         //
         void storefset(fsetv_t& fv, std::vector<feature_t>&featureset, sequential::Sequence *sq);
         float getbcost(int bias, int label, feature_t *featureset);

      private:
         Crf();
         Crf(const Crf&);
         Crf& operator=(const Crf&);
   };
}
# endif /* __CRFLEARNER__ */
