# include "rtcrflearn.hpp"
# include <sparsevect.h>

# define CRFLEARN_BUFSIZE 4096

using namespace RtCrf;
using namespace sequential;
using namespace toyutil;
using namespace std;

Crflearn::Crflearn(const char *tmpl, const char *corpus, unsigned int poolsize)
: Crf(tmpl,poolsize),corpus(corpus),bound(3),c(0.0001),cc(0),lambda(1),corrects(0),tags(0)
{
   this->delimit = " ";
}

Crflearn::~Crflearn()
{
   this->ac->release(this->pcache);
}

bool Crflearn::init()
{
   this->sqcolsize = this->checkcolsize(this->corpus.c_str(),this->delimit.c_str());
   this->labelcol = this->sqcolsize-1;
   if (this->valid)
   {
      this->ac->release(this->model);
      this->ac->release(this->pcache);
      this->features = new Dic(this->ac, CountUp);
      delete this->ufeatures;
      delete this->bfeatures;
      this->ufeatures = new Dic(this->ac, Index);
      this->bfeatures = new Dic(this->ac, Index);
   }
   /// feature extraction
   this->extfeature();
   /// feature rejection and making featurefunctions
   this->boundfeature();
   /// init model and pcache
   this->initmodel();
   /// first report
   this->report();
   this->valid = true;
   return true;
}

void Crflearn::setlabelcol(unsigned int labelcol)
{
   this->labelcol = labelcol;
}

void Crflearn::setbound(unsigned int bound)
{
   this->bound = bound;
}

void Crflearn::setpenalty(float p)
{
   this->c = p;
}

void Crflearn::setlambda(float lambda)
{
   this->lambda = lambda;
}

void Crflearn::inversef(nodeptr p, nodeptr nil, std::vector<char*>& f)
{
   if (p == nil)
   {
      return;
   }
   this->inversef(p->left,nil,f);
   this->inversef(p->right,nil,f);
   f[p->val] = p->key;

   return;
}

void Crflearn::save(const char *save)
{
   FILE *fp = NULL;
   if ((fp = fopen(save, "wb")) == NULL)
   {
      throw "Couldn't open save file";
   }
   fprintf(fp,"Params=%d\n",this->parameters);
   fprintf(fp,"Labels=%d\n",this->labelsize);
   fprintf(fp,"Start_Label\n");
   for (unsigned int i = 0; i < this->label2surf.size(); i++)
   {
      fprintf (fp, "[%d]=%s\n",i,this->label2surf[i]);
   }
   fprintf(fp,"End_Label\n");
   fprintf(fp, "Start_Params\n");
   fwrite(this->model,1,sizeof(float)*this->parameters,fp);
   fprintf (fp, "End_Params\n");
   std::vector<char*> ufs(this->ufeatures->getsize());
   std::vector<char*> bfs(this->bfeatures->getsize());
   nodeptr unil = this->ufeatures->getnil();
   nodeptr bnil = this->bfeatures->getnil();
   for (int i = 0; i < HASHSIZE; i++)
   {
      nodeptr *p = (this->ufeatures->table+i);
      if (*p == unil)
      {
         continue;
      }
      this->inversef(*p, unil, ufs);
   }
   for (int i = 0; i < HASHSIZE; i++)
   {
      nodeptr *p = (this->bfeatures->table+i);
      if (*p == bnil)
      {
         continue;
      }
      this->inversef(*p, bnil, bfs);
   }
   fprintf (fp, "Start_uFeatures\n");
   for (unsigned int i = 0; i < ufs.size(); i++)
   {
      fprintf (fp, "[%d]=%s\n",i,ufs[i]);
   }
   fprintf (fp, "End_uFeatures\n");
   fprintf (fp, "Start_bFeatures\n");
   for (unsigned int i = 0; i < bfs.size(); i++)
   {
      fprintf (fp, "[%d]=%s\n",i,bfs[i]);
   }
   fprintf (fp, "End_bFeatures\n");
   fclose(fp);
}

void Crflearn::lreport(unsigned int i)
{
   fprintf (stderr, "epoch:%3d\terr:%f(%d/%d)\n",i,
         1.-(float)this->corrects/this->tags,this->tags-this->corrects,this->tags);
   this->corrects = 0;
   this->tags = 0;
}

void Crflearn::decay(int t)
{
   double d = 1. + (double)t/this->instance;
   this->eta = 1./(this->lambda * d);
   this->cc += this->c/d;
}

void Crflearn::getclabels(Sequence *s, std::vector<int>& labels)
{
   int col = s->getRowSize();
   for (int i = 0; i < col; i++)
   {
      nodeptr *l = this->labels->get(s->getToken(i, this->labelcol));
      labels.push_back((*l)->val);
   }
}

void Crflearn::initlattice(fbnode **lattice,
      vector<feature_t>& featureset)
{
   int col = featureset.size();
   int row = this->labelsize;
   for (int i = 0; i < col; ++i)
   {
      for (int j = 0; j < row; ++j)
      {
         lattice[i][j]._alpha = 0.;
         lattice[i][j]._beta = 0.;
         lattice[i][j]._lcost = 0.;
         vector<pair<int,double> >::iterator uit = featureset[i].uf.begin();
         for (; uit != featureset[i].uf.end(); ++uit)
         {
            int id = (*uit).first;
            lattice[i][j]._lcost += *(this->model+id*row+j)*(*uit).second;
         }
      }
   }
}

float Crflearn::forward(fbnode **lattice,
      int col,
      vector<feature_t>& featureset)
{
   int row = this->labelsize;
   for (int i = 0; i < col; ++i)
   {
      for (int j = 0; j < row; ++j)
      {
         if (i == 0)
         {
            float cost = lattice[i][j]._lcost
               + this->getbcost(0, j, &featureset[i]);
            lattice[i][j]._alpha = Crf::logsumexp(lattice[i][j]._alpha, cost, true);
         }
         else
         {
            for (int k = 0; k < row; ++k)
            {
               float cost = lattice[i][j]._lcost
                  + this->getbcost(2*row+k*row, j, &featureset[i]);
               lattice[i][j]._alpha = 
                  Crf::logsumexp(lattice[i][j]._alpha,
                        lattice[i-1][k]._alpha+cost,(k==0));
            }
         }
      }
   }
   feature_t e;
   if (this->bonly)
   {
      pair<int,double> f;
      nodeptr *n = this->bfeatures->get("B");
      f.first = (*n)->val;
      f.second = 1.0;
      e.bf.push_back(f);
   }
   float z = 0;
   for (int j = 0; j < row; ++j)
   {
      float cost = this->getbcost(row, j, &e);
      z = Crf::logsumexp(z, lattice[col-1][j]._alpha+cost,(j == 0));
   }
   return z;
}

float Crflearn::backward(fbnode **lattice,
      int col,
      vector<feature_t>& featureset)
{
   int row = this->labelsize;
   feature_t e;
   if (this->bonly)
   {
      pair<int,double> f;
      nodeptr *n = this->bfeatures->get("B");
      f.first = (*n)->val;
      f.second = 1.0;
      e.bf.push_back(f);
   }
   for (int i = col-1; i >= 0; --i)
   {
      for (int j = 0; j < row; ++j)
      {
         if (i == col-1)
         {
            float cost = this->getbcost(row, j, &e);
            lattice[i][j]._beta = 
               Crf::logsumexp(lattice[i][j]._beta, cost, true);
         }
         else
         {
            for (int k = 0; k < row; ++k)
            {
               float cost = lattice[i+1][k]._lcost
                  + this->getbcost(2*row+j*row, k, &featureset[i+1]);
               lattice[i][j]._beta =
                  Crf::logsumexp(lattice[i][j]._beta,
                        lattice[i+1][k]._beta+cost,(k==0));
            }
         }
      }
   }
   float z = 0;
   for (int j = 0; j < row; ++j)
   {
      float cost = lattice[0][j]._lcost
         + this->getbcost(0, j, &featureset[0]);
      z = Crf::logsumexp(z, lattice[0][j]._beta+cost, (j==0));
   }
   return z;
}

void Crflearn::upbweight(int bias, int label, feature_t *bf, SparseVector *v, float expect)
{
   int lsize = this->labelsize;
   std::vector<pair<int,double> >::iterator bit = bf->bf.begin();
   for (; bit != bf->bf.end(); ++bit)
   {
      int id = bias + label + (*bit).first * (2*lsize+lsize*lsize);
      v->add(id,expect*(*bit).second);
   }
}

void Crflearn::upuweight(int label, feature_t *uf, SparseVector *v, float expect)
{
   int lsize = this->labelsize;
   std::vector<pair<int,double> >::iterator uit = uf->uf.begin();
   for (; uit != uf->uf.end(); ++uit)
   {
      int id = (*uit).first * lsize + label;
      v->add(id,expect*(*uit).second);
   }
}

void Crflearn::getcorrectv(std::vector<int>& corrects,
      std::vector<feature_t>& featureset,
      SparseVector *v)
{
   int lsize = this->labelsize;
   int col = corrects.size();
   for (int i = 0; i < col; i++)
   {
      int cl = corrects[i];
      /// unigram
      this->upuweight(cl, &featureset[i], v, 1.);
      /// bigram
      int bias = this->umid+1;
      if (i != 0)
      {
         int pl = corrects[i-1];
         bias += pl*lsize+(2*lsize);
      }
      this->upbweight(bias, cl, &featureset[i], v, 1.);
   }
   int bias = this->umid+1+lsize;
   feature_t e;
   if (this->bonly)
   {
      pair<int,double> f;
      nodeptr *n = this->bfeatures->get("B");
      f.first = (*n)->val;
      f.second = 1.0;
      e.bf.push_back(f);
   }
   this->upbweight(bias, corrects[col-1], &e, v, 1.);
}

/*
float Crflearn::getbcost(int bias,
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
*/

void Crflearn::getgradient(fbnode **lattice,
      int col,
      vector<feature_t>& featureset,
      SparseVector *v,
      float z,
      vector<int>& corrects)
{
   int row = this->labelsize;
   int bias = this->umid+1;
   this->tags += col;
   for (int i = 0; i < col; i++)
   {
      float max = 0;
      int predict = 0;
      for (int j = 0; j < row; j++)
      {
         float expect = 0.;
         if (i == 0) /// bos
         {
            float bcost = this->getbcost(0, j, &featureset[i]);
            expect = Crf::myexp(lattice[i][j]._beta
                  + lattice[i][j]._lcost
                  + bcost
                  - z);
            this->upbweight(bias, j, &featureset[i], v, -expect);
         }
         else
         {
            for (int k = 0; k < row; k++)
            {
               int b = bias + k*row + (2*row);
               float bcost = this->getbcost(2*row+k*row, j, &featureset[i]);
               float lex = Crf::myexp(lattice[i-1][k]._alpha
                     + lattice[i][j]._beta
                     + lattice[i][j]._lcost
                     + bcost
                     - z);
               //fprintf (stderr,"alpha: %f, beta: %f, lcost: %f, bcost: %f, z: %f, expect: %f\n",lattice[i-1][k]._alpha,
               //lattice[i][j]._beta, lattice[i][j]._lcost, bcost, z , lex);
               expect += lex;
               this->upbweight(b, j, &featureset[i], v, -lex);
            }
         }
         if (max < expect)
         {
            max = expect;  predict = j;
         }
         //fprintf(stderr,"c:%d\tp:%d\tl:%f\n",corrects[i],j,expect);
         this->upuweight(j, &featureset[i], v, -expect);
      }
      if (corrects[i] == predict)
      {
         this->corrects++;
      }
   }
   /// eos
   feature_t e;
   if (this->bonly)
   {
      pair<int,double> f;
      nodeptr *n = this->bfeatures->get("B");
      f.first = (*n)->val;
      f.second = 1.0;
      e.bf.push_back(f);
   }
   int be = bias + row;
   for (int j = 0; j < row; j++)
   {
      float bcost = this->getbcost(row,j,&e);
      float expect = Crf::myexp(lattice[col-1][j]._alpha
            + bcost
            - z);
      this->upbweight(be, j, &e, v, -expect);
   }
}

void Crflearn::l2_regularize(SparseVector *v)
{
   float p = 0;
   list<int>::iterator kit = v->keys.begin();
   for (; kit != v->keys.end(); ++kit)
   {
      p = this->cc - this->pcache[*kit];
      this->pcache[*kit] = this->cc;
      *(this->model+*kit) /= (1. + p);
   }
}

void Crflearn::l1_regularize(SparseVector *v)
{
   float p = 0;
   list<int>::iterator kit = v->keys.begin();
   for (; kit != v->keys.end(); ++kit)
   {
      p = this->cc - this->pcache[*kit];
      this->pcache[*kit] = this->cc;
      *(this->model+*kit) = Crf::sign(*(this->model+*kit)) *
         Crf::max(fabs(*(this->model+*kit))-p,0);
   }
}

void Crflearn::update(Sequence *sq, AllocMemdiscard *cache, unsigned int reg)
{
   int col = sq->getRowSize();
   int row = this->labelsize;
   SparseVector upv(cache);

   /// get correct labels
   std::vector<int> corrects;
   this->getclabels(sq, corrects);

   /// store feature set
   fsetv_t fv;
   vector<feature_t> featureset;
   this->storefset(fv, featureset, sq);

   /// build lattice
   fbnode **lattice = (fbnode**)cache->alloc(sizeof(fbnode*)*col);
   for (int c = 0; c < col; ++c)
   {
      lattice[c] = (fbnode*)cache->alloc(sizeof(fbnode)*row);
   }
   /// init lattice
   this->initlattice(lattice, featureset);

   /// forward-backward
   float z1 = this->forward(lattice, col, featureset);
   float z2 = this->backward(lattice, col, featureset);

   /*
      if (abs(z1-z2 > 1e-6))
      {
      fprintf(stderr,"%f %f\n", z1,z2);
      }
    */

   /// calc gradient
   this->getcorrectv(corrects, featureset, &upv);
   this->getgradient(lattice,
         col,
         featureset,
         &upv,
         Crf::max(z1,z2),
         corrects);

   /// update
   std::list<int>::iterator key_it = upv.keys.begin();
   for (; key_it != upv.keys.end(); ++key_it)
   {
      float d = upv.get(*key_it);
      *(this->model+*key_it) += this->eta * d;
   }

   /// regularization
   if (reg == 0)
   {
      this->l1_regularize(&upv);
   }
   else
   {
      this->l2_regularize(&upv);
   }
}

void Crflearn::learn(unsigned int iter, unsigned int reg)
{
   if (!this->valid)
   {
      throw "learner was not initialized";
   }
   AllocMemdiscard cache(this->cachesize);
   Sequence sq;
   sq.setColSize(this->sqcolsize);
   sq.setAllocSize(this->sqallocsize);
   sq.setArraySize(this->sqarraysize);
   sq.setDelimit(this->delimit.c_str());
   sq.init();
   int t = 0;
   for (unsigned int i = 0; i < iter; ++i)
   {
      FILE *fp = NULL;
      if ((fp = fopen(this->corpus.c_str(),"r")) == NULL)
      {
         throw "Couldn't open corpus";
      }
      while (feof(fp) == 0)
      {
         MyUtil::sqread(fp, &sq, CRFLEARN_BUFSIZE);
         if (sq.getRowSize() == 0)
         {
            continue;
         }
         this->decay(t++);
         this->update(&sq, &cache, reg);
         sq.clear();
         cache.reset();
      }
      fclose(fp);
      this->lreport(i);
   }
}

void Crflearn::report()
{
   fprintf (stderr,"labels: %d\n",this->labelsize);
   fprintf (stderr,"bound: %d\n", this->bound);
   fprintf (stderr,"ufeatures: %d\n", this->ufeatures->getsize());
   fprintf (stderr,"bfeatures: %d\n", this->bfeatures->getsize());
   fprintf (stderr,"instance: %d\n",this->instance);
   fprintf (stderr,"uparameters: %d\n",(int)this->umid+1);
   fprintf (stderr,"bparameters: %d\n",(int)this->bmid-(int)this->umid);
   fprintf (stderr,"model parameters: %d\n",(int)this->parameters);
}

void Crflearn::initmodel()
{
   int labelsize = this->labels->getsize();
   int uparams = this->ufeatures->getsize();
   int bparams = this->bfeatures->getsize();
   int params = labelsize * uparams +
      (2 * labelsize + labelsize * labelsize) * bparams;

   this->umid = labelsize*uparams-1;
   this->bmid = params - 1;
   this->parameters = params;
   this->labelsize = labelsize;

   /**
    * model parameters
    * +------------------------------------------------------+
    * | unigram feature functions | bigram feature functions |
    * +------------------------------------------------------+
    */
   this->model = (float*)this->ac->alloc(sizeof(float)*params);
   this->pcache = (float*)this->ac->alloc(sizeof(float)*params);
   for (int i = 0; i < params; ++i)
   {
      *(this->model+i) = 0.;
      *(this->pcache+i) = 0.;
   }

}

void Crflearn::storeff(nodeptr p, nodeptr nil)
{
   if (p == nil)
   {
      return;
   }
   this->storeff(p->left, nil);
   this->storeff(p->right, nil);
   if (p->val < (int)this->bound)
   {
      return;
   }
   char *c = p->key;
   if (*c == 'U') // unigram feature
   {
      this->ufeatures->insert(c);
   }
   else if (*c == 'B') // bigram feature
   {
      this->bfeatures->insert(c);
   }
   return;
}

void Crflearn::boundfeature()
{
   nodeptr nil = this->features->getnil();
   for (int i = 0; i < HASHSIZE; i++)
   {
      nodeptr *p = (this->features->table+i);
      if (*p == nil)
      {
         continue;
      }
      this->storeff(*p, nil);
   }
   delete this->features;
}

void Crflearn::extfeature()
{
   FILE *fp = NULL;
   if ((fp = fopen(this->corpus.c_str(),"r")) == NULL)
   {
      throw "Failed to open corpus";
   }
   Sequence sq;
   sq.setColSize(this->sqcolsize);
   sq.setAllocSize(this->sqallocsize);
   sq.setArraySize(this->sqarraysize);
   sq.setDelimit(this->delimit.c_str());
   sq.init();
   this->instance = 0;
   while (feof(fp) == 0)
   {
      MyUtil::sqread(fp,&sq,CRFLEARN_BUFSIZE);
      if (sq.getRowSize() == 0)
      {
         continue;
      }
      fsetv_t fv;
      unsigned int row = sq.getRowSize();
      fv.resize(row);
      for (unsigned int i = 0; i < row; ++i)
      {
         this->ft->parse(sq,i,fv);
      }
      this->finsert(fv);
      this->extlabel(&sq);
      ++this->instance;
      //sq.dump();
      //fsetvdump(fv);
      sq.clear();
   }
   fclose(fp);
}

void Crflearn::extlabel(Sequence *s)
{
   char *r = NULL;
   int row = s->getRowSize();
   for (int i = 0; i < row; ++i)
   {
      r = s->getToken(i, this->labelcol);
      nodeptr l = this->labels->insert(r);
      if (l != NULL)
      {
         this->label2surf.push_back(l->key);
      }
   }
}

void Crflearn::finsert(fsetv_t& fv)
{
   unsigned int size = fv.size();
   for (unsigned int i = 0; i < size; ++i)
   {
      unsigned int fsize = fv[i].size();
      for (unsigned int j = 0; j < fsize; ++j)
      {
         this->features->insert(fv[i][j].key.c_str());
      }
   }
}
