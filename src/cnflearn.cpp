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
# include <cnflearn.h>
# include <myutil.h>
# include <dic.h>
# include <sequence.h>
# include <allocmd.h>
# include <sparsevect.h>
# include <cstdlib>

# define CNF_BUFSIZE 1024
# define CNF_BLOCK 128

using namespace Cnf;
Cnflearn::Cnflearn(const char *tmpl, const char *corpus, unsigned int pool)
{
   this->tmpl = tmpl;
   this->corpus = corpus;
   /// valid
   this->valid = false;
   /// label col
   this->labelcol = 2;
   /// sqcolsize
   this->sqcolsize = 3;
   /// feature bound
   this->bound = 3;
   /// lambda
   this->lambda = 1;
   /// alpha w
   this->c[0] = 0.0001;
   /// alpha u
   this->c[1] = 0.0001;
   /// alpha t
   this->c[2] = 0.0001;
   /// alpha w cache
   this->cc[0] = 0;
   /// alpha u cache
   this->cc[1] = 0;
   /// alpha t cache
   this->cc[2] = 0;
   /// alloc pool
   this->ac = new PoolAlloc(CNF_BLOCK, pool);
   /// feature dic
   this->features = new Dic(this->ac, CountUp);
   /// unigram feature dic
   this->ufeatures = new Dic(this->ac, Index);
   /// bigram feature dic
   this->bfeatures = new Dic(this->ac, Index);
   /// label dic
   this->labels = new Dic(this->ac, Index);
   /// bonly flg
   this->bonly = false;
   /// bonly tmpl id
   this->botmpl = -1;
   /// template check
   this->tmplcheck();
   /// correct tags
   this->corrects = 0;
   /// tags
   this->tags = 0;
   /// cachesize
   this->cachesize = 1024*100000;
}

Cnflearn::~Cnflearn()
{
   this->ac->release(this->model);
   this->ac->release(this->pcache);
   delete this->ufeatures;
   delete this->bfeatures;
   delete this->labels;
   delete this->ac;
}

bool Cnflearn::init()
{
   if (this->valid)
   {
      this->ac->release(this->model);
      this->ac->release(this->pcache);
      /// feature dic
      this->features = new Dic(this->ac, CountUp);
   }
   /// feature extraction and encoding
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

void Cnflearn::setsqcol(unsigned int sqcolsize)
{
   this->sqcolsize = sqcolsize;
}

void Cnflearn::setlabelcol(unsigned int labelcol)
{
   this->labelcol = labelcol;
}

void Cnflearn::setcache(unsigned int cachesize)
{
   this->cachesize = cachesize;
}

void Cnflearn::setbound(unsigned int bound)
{
   this->bound = bound;
}

void Cnflearn::setpenalty(float w, float u, float t)
{
   /// alpha w
   this->c[0] = w;
   /// alpha u
   this->c[1] = u;
   /// alpha t
   this->c[2] = t;
}

void Cnflearn::setlambda(float lambda)
{
   this->lambda = lambda;
}

void Cnflearn::decay(int t)
{
   double d = 1. + (double)t/this->instance;
   this->eta = 1./(this->lambda * d);
   this->cc[0] += this->c[0]/d;
   this->cc[1] += this->c[1]/d;
   this->cc[2] += this->c[2]/d;
}

void Cnflearn::getclabels(Sequence *s, std::vector<int>& labels)
{
   int col = s->getRowSize();
   for (int i = 0; i < col; i++)
   {
      nodeptr *l = this->labels->get(s->getToken(i, this->labelcol));
      labels.push_back((*l)->val);
   }
}

void Cnflearn::storefset(Sequence *sq,
      std::vector<feature_t>& featureset)
{
   int col = sq->getRowSize();
   featureset.resize(col);
   // feature extraction
   nodeptr unil = this->ufeatures->getnil();
   nodeptr bnil = this->bfeatures->getnil();
   for (int i = 0; i < col; i++)
   {
      FILE *fp = NULL;
      if ((fp = fopen(this->tmpl.c_str(),"r")) == NULL)
      {
         fprintf (stderr,"Couldn't open %s\n", this->tmpl.c_str());
         exit (1);
      }
      char buf[CNF_BUFSIZE];
      int tmpl = 0;
      while (fgets(buf, CNF_BUFSIZE, fp) != NULL)
      {
         MyUtil::chomp(buf);
         if (MyUtil::IsCommentOut(buf))
         {
            continue;
         }
         char *f = this->expand(buf, sq, i);
         //int tmpl = this->f2t[f];
         if (*f == 'U') /// unigram feature
         {
            nodeptr *n = this->ufeatures->get(f);
            if (*n != unil)
            {
               featureset[i].uf.push_back((*n)->val);
               featureset[i].ut.push_back(tmpl);
            }
         }
         else if (*f == 'B') /// bigram feature
         {
            nodeptr *n = this->bfeatures->get(f);
            if (*n != bnil)
            {
               featureset[i].bf.push_back((*n)->val);
               featureset[i].bt.push_back(tmpl);
            }
         }
         this->ac->release(f);
         tmpl++;
      }
      fclose(fp);
   }
}

void Cnflearn::initlattice(fbnode **lattice,
      std::vector<feature_t>& featureset)
{
   int col = featureset.size();
   int row = this->labels->getsize();
   for (int i = 0; i < col; i++)
   {
      for (int j = 0; j < row; j++)
      {
         lattice[i][j]._alpha = 0.;
         lattice[i][j]._beta = 0.;
         lattice[i][j]._lcost = 0.;
         std::vector<int>::iterator uit = featureset[i].uf.begin();
         std::vector<int>::iterator utit = featureset[i].ut.begin();
         for (; uit != featureset[i].uf.end(); uit++, utit++)
         {
            int id = *uit;
            int tmpl = *utit;
            float w = *(this->model+id*row+j);
            if (this->fwit[tmpl] == this->fwit[tmpl+1])
            {
               lattice[i][j]._lcost += w;
            }
            else
            {
               float a = 0;
               for (int k = this->fwit[tmpl]; k < this->fwit[tmpl+1]; k++)
               {
                  a += *(this->model+k);
               }
               lattice[i][j]._lcost += w * Cnflearn::logistic(a);
            }
         }
      }
   }
}

float Cnflearn::getbcost(int bias,
      int label,
      feature_t *featureset)
{
   int lsize = this->labels->getsize();
   int b = this->umid+1+bias+label;
   float ret = 0;
   std::vector<int>::iterator bit = featureset->bf.begin();
   std::vector<int>::iterator btit = featureset->bt.begin();
   for (; bit != featureset->bf.end(); bit++,btit++)
   {
      int id = b + *bit * (2*lsize+lsize*lsize);
      int tmpl = *btit;
      float w = *(this->model+id);
      if (this->fwit[tmpl] == this->fwit[tmpl+1])
      {
         ret += w;
      }
      else
      {
         float a = 0;
         for (int k = this->fwit[tmpl]; k < this->fwit[tmpl+1]; k++)
         {
            a += *(this->model+k);
         }
         ret += w * Cnflearn::logistic(a);
      }
   }
   return ret;
}

float Cnflearn::forward(fbnode **lattice,
      int col,
      std::vector<feature_t>& featureset)
{
   int row = this->labels->getsize();
   for (int i = 0; i < col; i++)
   {
      for (int j = 0; j < row; j++)
      {
         if (i == 0)
         {
            float cost = lattice[i][j]._lcost
               + this->getbcost(0, j, &featureset[i]);
            lattice[i][j]._alpha = Cnflearn::logsumexp(lattice[i][j]._alpha, cost, true);
         }
         else
         {
            for (int k = 0; k < row; k++)
            {
               float cost = lattice[i][j]._lcost
                  + this->getbcost(2*row+k*row, j, &featureset[i]);
               lattice[i][j]._alpha =
                  Cnflearn::logsumexp(lattice[i][j]._alpha,
                        lattice[i-1][k]._alpha+cost,(k==0));
            }
         }
      }
   }
   feature_t e;
   if (this->bonly)
   {
      nodeptr *n = this->bfeatures->get("B");
      e.bf.push_back((*n)->val);
      e.bt.push_back(this->botmpl);
   }
   float z = 0;
   for (int j = 0; j < row; j++)
   {
      float cost = this->getbcost(row, j, &e);
      z = Cnflearn::logsumexp(z, lattice[col-1][j]._alpha+cost, (j == 0));
   }
   return z;
}

float Cnflearn::backward(fbnode **lattice,
      int col,
      std::vector<feature_t>& featureset)
{
   int row = this->labels->getsize();
   feature_t e;
   if (this->bonly)
   {
      nodeptr *n = this->bfeatures->get("B");
      e.bf.push_back((*n)->val);
      e.bt.push_back(this->botmpl);
   }
   for (int i = col-1; i >= 0; i--)
   {
      for (int j = 0; j < row; j++)
      {
         if (i == col-1)
         {
            float cost = this->getbcost(row, j, &e);
            lattice[i][j]._beta =
               Cnflearn::logsumexp(lattice[i][j]._beta, cost, true);
         }
         else
         {
            for (int k = 0; k < row; k++)
            {
               float cost = lattice[i+1][k]._lcost
                  + this->getbcost(2*row+j*row, k, &featureset[i+1]);
               lattice[i][j]._beta =
                  Cnflearn::logsumexp(lattice[i][j]._beta,
                        lattice[i+1][k]._beta+cost,(k==0));
            }
         }
      }
   }
   float z = 0;
   for (int j = 0; j < row; j++)
   {
      float cost = lattice[0][j]._lcost
         + this->getbcost(0, j, &featureset[0]);
      z = Cnflearn::logsumexp(z, lattice[0][j]._beta+cost, (j==0));
   }
   return z;
}

void Cnflearn::upbweight(int bias, int label, feature_t *bf, SparseVector *v, float expect)
{
   int lsize = this->labels->getsize();
   std::vector<int>::iterator bit = bf->bf.begin();
   std::vector<int>::iterator btit = bf->bt.begin();
   for (; bit != bf->bf.end(); bit++,btit++)
   {
      int id = bias + label + *bit * (2*lsize+lsize*lsize);
      int tmpl = *btit;
      float w = *(this->model+id);
      if (this->fwit[tmpl] == this->fwit[tmpl+1])
      {
         v->add(id,expect);
      }
      else
      {
         float a = 0.;
         for (int k = this->fwit[tmpl]; k < this->fwit[tmpl+1]; k++)
         {
            a += *(this->model+k);
         }
         float h = Cnflearn::logistic(a);
         v->add(id, h*expect);
         /// for gate function parameter
         float th = expect*h*(1.-h)*w;
         for (int k = this->fwit[tmpl]; k < this->fwit[tmpl+1]; k++)
         {
            v->add(id, th);
         }
      }
   }
}

void Cnflearn::upuweight(int label, feature_t *uf, SparseVector *v, float expect)
{
   int lsize = this->labels->getsize();
   std::vector<int>::iterator uit = uf->uf.begin();
   std::vector<int>::iterator utit = uf->ut.begin();
   for (; uit != uf->uf.end(); uit++, utit++)
   {
      int id = *uit * lsize + label;
      int tmpl = *utit;
      float a = 0.;
      for (int k = this->fwit[tmpl]; k < this->fwit[tmpl+1]; k++)
      {
         a += *(this->model+k);
      }
      float h = Cnflearn::logistic(a);
      v->add(id, h*expect);
      /// for gate function parameter
      float th = expect*h*(1.-h)* *(this->model+id);
      for (int k = this->fwit[tmpl]; k < this->fwit[tmpl+1]; k++)
      {
         v->add(k, th);
      }
   }
}

void Cnflearn::getcorrectv(std::vector<int>& corrects,
      std::vector<feature_t>& featureset,
      SparseVector *v)
{
   int lsize = this->labels->getsize();
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
      nodeptr *n = this->bfeatures->get("B");
      e.bf.push_back((*n)->val);
      e.bt.push_back(this->botmpl);
   }
   this->upbweight(bias, corrects[col-1], &e, v, 1.);
}

void Cnflearn::getgradient(fbnode **lattice,
      int col,
      std::vector<feature_t>& featureset,
      SparseVector *v,
      float z,
      std::vector<int>& corrects)
{
   int row = this->labels->getsize();
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
            expect = Cnflearn::myexp(lattice[i][j]._beta
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
               float lex = Cnflearn::myexp(lattice[i-1][k]._alpha
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
      nodeptr *n = this->bfeatures->get("B");
      e.bf.push_back((*n)->val);
      e.bt.push_back(this->botmpl);
   }
   int be = bias + row;
   for (int j = 0; j < row; j++)
   {
      float bcost = this->getbcost(row,j,&e);
      float expect = Cnflearn::myexp(lattice[col-1][j]._alpha
            + bcost
            - z);
      this->upbweight(be, j, &e, v, -expect);
   }
}

void Cnflearn::update(Sequence *sq, AllocMemdiscard *cache, unsigned int reg)
{
   int col = sq->getRowSize();
   int row = this->labels->getsize();
   SparseVector upv(cache);

   /// get correct labels
   std::vector<int> corrects;
   this->getclabels(sq, corrects);

   /// store feature set
   std::vector<feature_t> featureset;
   this->storefset(sq, featureset);

   /// build lattice
   fbnode **lattice = (fbnode**)cache->alloc(sizeof(fbnode*)*col);
   for (int c = 0; c < col ; c++)
   {
      lattice[c] = (fbnode*)cache->alloc(sizeof(fbnode)*row);
   }
   /// init lattice
   this->initlattice(lattice, featureset);

   /// forward-backward
   float z1 = this->forward(lattice, col, featureset);
   float z2 = this->backward(lattice, col, featureset);

   // if (std::abs(z1-z2) < 1e-6)
   // fprintf(stderr,"%f %f\n",z1,z2);

   /// calc gradient
   this->getcorrectv(corrects, featureset, &upv);
   this->getgradient(lattice,
         col,
         featureset,
         &upv,
         Cnflearn::max(z1,z2),
         corrects);
   /// update
   std::list<int>::iterator key_it = upv.keys.begin();
   for (; key_it != upv.keys.end(); key_it++)
   {
      float d = upv.get(*key_it);
      *(this->model+*key_it) += this->eta * d;
   }

   if (reg == 0)
   {
      this->l1_regularize(&upv);
   }
   else
   {
      this->l2_regularize(&upv);
   }
}

void Cnflearn::l2_regularize(SparseVector *v)
{
   float p = 0;
   std::list<int>::iterator kit = v->keys.begin();
   for (; kit != v->keys.end(); kit++)
   {
      if (*kit > (int)this->bmid)
      {
         p = this->cc[2] - this->pcache[*kit];
         this->pcache[*kit] = this->cc[2];
      }
      else if (*kit > (int)this->umid)
      {
         p = this->cc[1] - this->pcache[*kit];
         this->pcache[*kit] = this->cc[1];
      }
      else
      {
         p = this->cc[0] - this->pcache[*kit];
         this->pcache[*kit] = this->cc[0];
      }
      *(this->model+*kit) /= (1.+p);
   }
}

void Cnflearn::l1_regularize(SparseVector *v)
{
   float p = 0;
   std::list<int>::iterator kit = v->keys.begin();
   for (; kit != v->keys.end(); kit++)
   {
      if (*kit > (int)this->bmid)
      {
         p = this->cc[2] - this->pcache[*kit];
         this->pcache[*kit] = this->cc[2];
      }
      else if (*kit > (int)this->umid)
      {
         p = this->cc[1] - this->pcache[*kit];
         this->pcache[*kit] = this->cc[1];
      }
      else
      {
         p = this->cc[0] - this->pcache[*kit];
         this->pcache[*kit] = this->cc[0];
      }
      *(this->model+*kit) = Cnflearn::sign(*(this->model+*kit))
         * Cnflearn::max(std::fabs(*(this->model+*kit))-p, 0.);
   }
}

void Cnflearn::learn(unsigned int iter, unsigned int reg)
{
   if (!this->valid)
   {
      fprintf (stderr, "It's not initialized\n");
      exit(1);
   }
   AllocMemdiscard cache(this->cachesize);
   Sequence sq;
   sq.setColSize(this->sqcolsize);
   sq.init();
   int t = 0;
   for (unsigned int i = 0; i < iter; i++)
   {
      FILE *fp = NULL;
      if ((fp = fopen(this->corpus.c_str(), "r")) == NULL)
      {
         fprintf (stderr, "Couldn't open %s\n", this->corpus.c_str());
         exit (1);
      }
      while (feof(fp) == 0)
      {
         MyUtil::sqread(fp, &sq, CNF_BUFSIZE);
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

void Cnflearn::lreport(unsigned int i)
{
   fprintf (stderr, "epoch:%3d\terr:%f(%d/%d)\n",i,
         1.-(float)this->corrects/this->tags,this->tags-this->corrects,this->tags);
   this->corrects = 0;
   this->tags = 0;
}

void Cnflearn::report()
{
   fprintf (stderr,"labels: %d\n",this->labels->getsize());
   fprintf (stderr,"features: %d\n",this->features->getsize());
   fprintf (stderr,"bound: %d\n", this->bound);
   fprintf (stderr,"ufeatures: %d\n", this->ufeatures->getsize());
   fprintf (stderr,"bfeatures: %d\n", this->bfeatures->getsize());
   fprintf (stderr,"instance: %d\n",this->instance);
   fprintf (stderr,"gate functions: %d\n", (int)this->fwit.size()-1);
   fprintf (stderr,"parameters of gate functions: %d\n",this->nk);
   fprintf (stderr,"uparameters: %d\n",(int)this->umid+1);
   fprintf (stderr,"bparameters: %d\n",(int)this->bmid-(int)this->umid);
   fprintf (stderr,"model parameters: %d\n",(int)this->parameters);
   /*
      for (int i = 0; i < (int)this->fwit.size()-1; i++)
      {
      fprintf (stdout,"gate %d : param %d - %d\n",i,this->fwit[i],this->fwit[i+1]);
      }
    */
}

void Cnflearn::initmodel()
{
   int labelsize = this->labels->getsize();
   int uparams = this->ufeatures->getsize();
   int bparams = this->bfeatures->getsize();
   int gparams = this->nk;
   int params = labelsize * uparams
      + (2*labelsize+labelsize*labelsize)*bparams
      + gparams;

   this->umid = labelsize*uparams-1;
   this->bmid = this->umid
      + (2*labelsize+labelsize*labelsize)*bparams;
   this->parameters = params;
   /**
    * model parameters
    * +-----------------------------------------------------------------------+
    * | unigram feature functions | bigram feature functions | gate functions |
    * +-----------------------------------------------------------------------+
    */
   this->model = (float*)this->ac->alloc(sizeof(float)*params);
   this->pcache = (float*)this->ac->alloc(sizeof(float)*params);

   for (int i = 0; i < params-gparams; i++)
   {
      *(this->model+i) = 0.;
      *(this->pcache+i) = 0.;
   }
   int t = 0;
   for (int j = params-gparams; j < params; j++)
   {
      *(this->model+j) = this->theta[t++];
      *(this->pcache+j) = 0.;
   }
   int b = this->parameters - this->nk;
   for (int g = 0; g < (int)this->fwit.size(); g++)
   {
      this->fwit[g] += b;
   }
}

void Cnflearn::storeff(nodeptr p, nodeptr nil)
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

void Cnflearn::boundfeature()
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

char* Cnflearn::expand(char *tp, Sequence *s, int current)
{
   char f[CNF_BUFSIZE] = "\0";
   for (char *p = tp; *p != '\0'; p++)
   {
      if (*p == '%' && *(p+1) == 'x' && *(p+2) == '[')
      {
         *p++ = '\0';
         std::strcat(f,tp);
         tp = p+2;
         for (; *p != ','; p++) ;
         *p++ = '\0';
         int row = atoi(tp);
         tp = p;
         for (; *p != ','; p++) ;
         *p++ = '\0';
         int col = atoi(tp);
         tp = p;
         for (; *p != ']'; p++) ;
         *p++ = '\0';
         std::strcat(f, s->getToken(current+row,col));
         tp = p;
      }
   }
   if (*tp != '\0')
   {
      std::strcat(f, tp);
   }
   char *feature = (char*)this->ac->alloc(sizeof(char)*std::strlen(f)+1);
   std::strcpy(feature,f);
   return feature;
}

void Cnflearn::extract(Sequence *s)
{
   unsigned int size = s->getRowSize();
   for (int i = 0; i < (int)size; i++)
   {
      FILE *fp = NULL;
      if ((fp = fopen(this->tmpl.c_str(),"r")) == NULL)
      {
         fprintf (stderr,"Couldn't open %s\n",this->tmpl.c_str());
         exit (1);
      }
      char buf[CNF_BUFSIZE];
      int tmpl = 0;
      while (fgets(buf, CNF_BUFSIZE, fp) != NULL)
      {
         MyUtil::chomp(buf);
         if (MyUtil::IsCommentOut(buf))
         {
            continue;
         }
         char *f = this->expand(buf, s, i);
         nodeptr n = this->features->insert(f);
         this->ac->release(f);
         tmpl++;
      }
      fclose(fp);
   }
}

void Cnflearn::extlabel(Sequence *s)
{
   char *r = NULL;
   int row = s->getRowSize();
   for (int i = 0; i < row; i++)
   {
      r = s->getToken(i, this->labelcol);
      nodeptr l = this->labels->insert(r);
      if (l != NULL)
      {
         this->label2surf.push_back(l->key);
      }
   }
}

void Cnflearn::extfeature()
{
   FILE *fp = NULL;
   if ((fp = fopen(this->corpus.c_str(),"r")) == NULL)
   {
      fprintf (stderr,"Couln't open %s\n", this->corpus.c_str());
      exit (1);
   }
   Sequence sq;
   sq.setColSize(this->sqcolsize);
   sq.init();
   this->instance = 0;
   while(feof(fp) == 0)
   {
      MyUtil::sqread(fp,&sq,CNF_BUFSIZE);
      if (sq.getRowSize() == 0)
      {
         continue;
      }
      this->extlabel(&sq);
      this->extract(&sq);
      this->instance++;
      sq.clear();
   }
   fclose(fp);
}

bool Cnflearn::check(char *tp)
{
   if (*tp != 'U' && *tp != 'B')
   {
      return false;
   }
   bool okay = true;
   for (char *p = tp; *p != '\0'; p++)
   {
      if (*p == '%' && *(p+1) == 'x' && *(p+2) == '[')
      {
         okay = false;
         tp = p+2;
         for (; *p != ',' && *p != '\0'; p++) ;
         if (*p == '\0')
            break;
         *p++ = '\0';
         // row
         atoi(tp);
         tp = p;
         for (; *p != ',' && *p != '\0'; p++) ;
         if (*p == '\0')
            break;
         *p++ = '\0';
         // col
         atoi(tp);
         tp = p;
         for (; *p != ']' && *p != '\0'; p++) ;
         if (*p == '\0')
            break;
         *p++ = '\0';
         // weight
         float t = atof(tp);
         // count up number of weights
         this->nk++;
         // init theta parameter
         this->theta.push_back(t);
         okay = true;
      }
   }

   return okay;
}

bool Cnflearn::tmplcheck()
{
   FILE *fp = NULL;
   if ((fp = fopen(this->tmpl.c_str(),"r")) == NULL)
   {
      fprintf (stderr,"Couldn't open %s\n",this->tmpl.c_str());
      exit (1);
   }
   this->nk = 0;
   int tmpl = 0;
   char buf[CNF_BUFSIZE];
   while (fgets(buf, CNF_BUFSIZE, fp) != NULL)
   {
      MyUtil::chomp(buf);
      if (MyUtil::IsCommentOut(buf))
      {
         continue;
      }
      this->fwit.push_back(this->nk);
      if (!this->check(buf))
      {
         fprintf (stderr, "ERR: Invalid template %s\n", buf);
         exit (1);
      }
      if (std::strcmp(buf,"B") == 0)
      {
         this->bonly = true;
         this->botmpl = tmpl;
      }
      tmpl++;
   }
   fclose(fp);
   this->fwit.push_back(this->nk);

   return true;
}

void Cnflearn::inversef(nodeptr p, nodeptr nil, std::vector<char*>& f)
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

void Cnflearn::save(const char *save)
{
   FILE *fp = NULL;
   if ((fp = fopen(save,"wb")) == NULL)
   {
      fprintf (stderr, "Couldn't open %s\n",save);
      exit(1);
   }
   fprintf (fp, "Params=%d\n",this->parameters);
   fprintf (fp, "Labels=%d\n",this->labels->getsize());
   fprintf (fp, "Start_Label\n");
   for (unsigned int i = 0; i < this->label2surf.size(); i++)
   {
      fprintf (fp, "[%d]=%s\n",i,this->label2surf[i]);
   }
   fprintf (fp, "End_Label\n");
   fprintf (fp, "Start_Fwit\n");
   for (unsigned int i = 0; i < this->fwit.size(); i++)
   {
      fprintf (fp, "[%d]=%d\n",i,this->fwit[i]);
   }
   fprintf (fp, "End_Fwit\n");
   fprintf (fp, "Start_Params\n");
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
