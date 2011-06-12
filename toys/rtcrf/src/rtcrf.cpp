# include "rtcrf.hpp"
# include <dic.h>
# include <sequence.h>
# include <allocmd.h>
# include <sparsevect.h>
# include <cstdlib>

# define CRF_BUFSIZE 4096
# define CRF_BLOCK 256

using namespace RtCrf;
using namespace sequential;
using namespace toyutil;
using namespace std;

Crf::Crf(const char *tmpl,unsigned int pool)
: tmpl(tmpl),sqcolsize(3),sqarraysize(1000),sqallocsize(4096*1000),valid(false),bonly(false),cachesize(1024*100000),delimit(" ")
{
   this->ac = new PoolAlloc(CRF_BLOCK, pool);
   this->features = new Dic(this->ac, CountUp);
   this->ufeatures = new Dic(this->ac, Index);
   this->bfeatures = new Dic(this->ac, Index);
   this->labels = new Dic(this->ac, Index);
   this->ft = new ftemplate(this->tmpl.c_str());
}

Crf::~Crf()
{
   this->ac->release(this->model);
   delete this->ufeatures;
   delete this->bfeatures;
   delete this->labels;
   delete this->ac;
}

unsigned int Crf::checkcolsize(const char *file, const char *delim)
{
   FILE *fp = NULL;
   if ((fp = fopen(file,"r")) == NULL)
   {
      throw "Failed to open file";
   }
   char buf[CRF_BUFSIZE];
   unsigned int colsize = 0;
   while (fgets(buf,CRF_BUFSIZE,fp) != NULL)
   {
      MyUtil::chomp(buf);
      if (buf[0] == '\0')
      {
         continue;
      }
      unsigned int c = 1;
      const char *p = buf;
      unsigned int shift = 0;
      while (shift = MyUtil::getByteUtf8(p))
      {
         if (strncmp(p, delim, strlen(delim)) == 0)
         {
            ++c;
         }
         p += shift;
         if (strlen(p) == 0)
         {
            break;
         }
      }
      if (colsize != 0 && c != colsize)
      {
         string err = "colsize unmatched:";
         err += buf;
         throw err.c_str();
      }
      colsize = c;
   }
   fclose(fp);
   return colsize;
}

float Crf::getbcost(int bias,
      int label,
      feature_t *featureset)
{
   int lsize = this->labelsize;
   int b = this->umid+1+bias+label;
   float ret = 0;
   vector<pair<int,double> >::iterator bit = featureset->bf.begin();
   for (; bit != featureset->bf.end(); ++bit)
   {
      int id = b + (*bit).first * (2*lsize+lsize*lsize);
      ret += *(this->model+id)*(*bit).second;
   }
   return ret;
}

void Crf::storefset(fsetv_t& fv, vector<feature_t>& featureset,Sequence *sq)
{
   unsigned int row = sq->getRowSize();
   fv.resize(row);
   featureset.resize(row);
   for (unsigned int i = 0; i < row; ++i)
   {
      this->ft->parse(*sq,i,fv);
   }
   for (unsigned int i = 0; i < row; ++i)
   {
      unsigned int fsize = fv[i].size();
      for (unsigned int j = 0; j < fsize; ++j)
      {
         if (fv[i][j].ngram == 1)
         {
            nodeptr unil = this->ufeatures->getnil();
            nodeptr *n = this->ufeatures->get(fv[i][j].key.c_str());
            if (*n != unil)
            {
               pair<int, double> f;
               f.first = ((*n)->val);
               f.second = fv[i][j].val;
               featureset[i].uf.push_back(f);
            }
         }
         else if (fv[i][j].ngram == 2)
         {
            nodeptr bnil = this->ufeatures->getnil();
            nodeptr *n = this->bfeatures->get(fv[i][j].key.c_str());
            if (*n != bnil)
            {
               pair<int, double> f;
               f.first = ((*n)->val);
               f.second = fv[i][j].val;
               featureset[i].bf.push_back(f);
            }
         }
         /// ここでチェックすると繰り返しが多いのであんまり良くない
         if (fv[i][j].key == "B")
         {
            this->bonly = true;
         }
      }
   }
}

void Crf::setdelimit(const char *d)
{
   this->delimit = d;
}

void Crf::setcache(unsigned int cachesize)
{
   this->cachesize = cachesize;
}

void Crf::setsqallocsize(unsigned int sqallocsize)
{
   this->sqallocsize = sqallocsize;
}

void Crf::setsqarraysize(unsigned int sqarraysize)
{
   this->sqarraysize = sqarraysize;
}

