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
# include <cnftagger.h>
# include <algorithm>

# define CNF_BUFSIZE 1024
# define CNF_BLOCK 128

using namespace Cnf;
Cnftagger::Cnftagger(const char *tmpl, unsigned int pool)
{
   this->tmpl = tmpl;
   /// sqcolsize
   this->sqcolsize = 3;
   /// alloc pool
   this->ac = new PoolAlloc(CNF_BLOCK, pool);
   /// unigram feature dic
   this->ufeatures = new Dic(this->ac, Index);
   /// bigram feature dic
   this->bfeatures = new Dic(this->ac, Index);
   /// bonly flg
   this->bonly = false;
   /// bonly tmpl id
   this->botmpl = -1;
   /// template check
   this->tmplcheck();
   /// cache size
   this->cachesize = 1024*100000;

   this->valid = false;
}

Cnftagger::~Cnftagger()
{
   if (this->valid)
   {
      this->ac->release(this->model);
   }
   delete this->ufeatures;
   delete this->bfeatures;
   delete this->ac;
}

void Cnftagger::setsqcol(unsigned int sqcolsize)
{
   this->sqcolsize = sqcolsize;
}

void Cnftagger::setcache(unsigned int cachesize)
{
   this->cachesize = cachesize;
}

void Cnftagger::clear()
{
   if (this->valid)
   {
      this->ac->release(this->model);
      delete this->ufeatures;
      delete this->bfeatures;
      this->ufeatures = new Dic(this->ac, Index);
      this->bfeatures = new Dic(this->ac, Index);
      this->valid = false;
   }
}

/*
void Cnftagger::setf2t(FILE *fp)
{
   char buf[CNF_BUFSIZE];
   while (fgets(buf, CNF_BUFSIZE, fp) != NULL)
   {
      MyUtil::chomp(buf);
      if (MyUtil::IsCommentOut(buf))
      {
         continue;
      }
      if (std::strncmp(buf,"End_FtoTmpl",11) == 0)
      {
         break;
      }
      char f[CNF_BUFSIZE];
      int t = 0;
      sscanf(buf,"%d:%s",&t,f);
      this->f2t[f] = t;
   }
}
*/

void Cnftagger::setlabel(FILE *fp)
{
   char buf[CNF_BUFSIZE];
   while (fgets(buf, CNF_BUFSIZE, fp) != NULL)
   {
      MyUtil::chomp(buf);
      if (MyUtil::IsCommentOut(buf))
      {
         continue;
      }
      if (std::strncmp(buf,"End_label",9) == 0)
      {
         break;
      }
      char l[CNF_BUFSIZE];
      int id = -1;
      sscanf(buf,"[%d]=%s",&id,l);
      if (id < 0)
      {
         fprintf (stderr,"Unknown parameter %s\n",buf);
         exit (1);
      }
      this->label2surf[id] = l;
   }
}

void Cnftagger::setfwit(FILE *fp)
{
   char buf[CNF_BUFSIZE];
   while (fgets(buf, CNF_BUFSIZE, fp) != NULL)
   {
      MyUtil::chomp(buf);
      if (MyUtil::IsCommentOut(buf))
      {
         continue;
      }
      if (std::strncmp(buf,"End_Fwit",8) == 0)
      {
         break;
      }
      int tid = -1;
      int thetaid = -1;
      sscanf(buf,"[%d]=%d",&tid,&thetaid);
      if (tid < 0 || thetaid < 0)
      {
         fprintf (stderr,"Unknown parameter %s\n",buf);
         exit (1);
      }
      this->fwit.push_back(thetaid);
      if (this->fwit[tid] != thetaid)
      {
         fprintf (stderr,"Unknown parameter %s\n",buf);
         exit (1);
      }
   }
}

void Cnftagger::setparams(FILE *fp)
{
   char buf[CNF_BUFSIZE];
   while (fgets(buf, CNF_BUFSIZE, fp) != NULL)
   {
      MyUtil::chomp(buf);
      if (MyUtil::IsCommentOut(buf))
      {
         continue;
      }
      if (std::strncmp(buf,"End_Params",10) == 0)
      {
         break;
      }
      int id = -1;
      float param = 0.;
      sscanf(buf,"[%d]=%f",&id,&param);
      if (id < 0)
      {
         fprintf (stderr,"Unknown parameter %s\n",buf);
         exit (1);
      }
      this->model[id] = param;
   }
}

void Cnftagger::setufeatures(FILE *fp)
{
   char buf[CNF_BUFSIZE];
   while (fgets(buf, CNF_BUFSIZE, fp) != NULL)
   {
      MyUtil::chomp(buf);
      if (MyUtil::IsCommentOut(buf))
      {
         continue;
      }
      if (std::strncmp(buf,"End_uFeatures",13) == 0)
      {
         break;
      }
      int id = -1;
      char f[CNF_BUFSIZE];
      sscanf(buf,"[%d]=%s",&id,f);
      if (id < 0)
      {
         fprintf (stderr,"Unknown parameter %s\n",buf);
         exit (1);
      }
      nodeptr n = this->ufeatures->insert(f);
      if (n->val != id)
      {
         fprintf (stderr,"Unknown parameter %s\n",buf);
         exit (1);
      }
   }
}

void Cnftagger::setbfeatures(FILE *fp)
{
   char buf[CNF_BUFSIZE];
   while (fgets(buf, CNF_BUFSIZE, fp) != NULL)
   {
      MyUtil::chomp(buf);
      if (MyUtil::IsCommentOut(buf))
      {
         continue;
      }
      if (std::strncmp(buf,"End_bFeatures",13) == 0)
      {
         break;
      }
      int id = -1;
      char f[CNF_BUFSIZE];
      sscanf(buf,"[%d]=%s",&id,f);
      if (id < 0)
      {
         fprintf (stderr,"Unknown parameter %s\n",buf);
         exit (1);
      }
      nodeptr n = this->bfeatures->insert(f);
      if (n->val != id)
      {
         fprintf (stderr,"Unknown parameter %s\n",buf);
         exit (1);
      }
   }
}

void Cnftagger::read(const char *model)
{
   if (this->valid)
   {
      fprintf (stderr, "Model is not cleared\n");
      exit(1);
   }
   FILE *fp = NULL;
   if ((fp = fopen(model,"r")) == NULL)
   {
      fprintf (stderr,"Couldn't open %s\n",model);
      exit(1);
   }
   char buf[CNF_BUFSIZE];
   while (fgets(buf, CNF_BUFSIZE, fp) != NULL)
   {
      MyUtil::chomp(buf);
      if (MyUtil::IsCommentOut(buf))
      {
         continue;
      }
      if (std::strncmp(buf,"Params=",7) == 0)
      {
         int params = 0;
         sscanf (buf+7,"%d",&params);
         this->model = (float*)this->ac->alloc(sizeof(float)*params);
      }
      else if (std::strncmp(buf,"Labels=",7) == 0)
      {
         sscanf(buf+7,"%d",&this->labelsize);
         this->label2surf.resize(this->labelsize);
      }
      //else if (std::strncmp(buf,"Start_FtoTmpl",13) == 0)
      //{
      //   this->setf2t(fp);
      //}
      else if (std::strncmp(buf,"Start_Label",11) == 0)
      {
         this->setlabel(fp);
      }
      else if (std::strncmp(buf,"Start_Fwit",10) == 0)
      {
         this->setfwit(fp);
      }
      else if (std::strncmp(buf,"Start_Params",12) == 0)
      {
         this->setparams(fp);
      }
      else if (std::strncmp(buf,"Start_uFeatures",15) == 0)
      {
         this->setufeatures(fp);
      }
      else if (std::strncmp(buf,"Start_bFeatures",15) == 0)
      {
         this->setbfeatures(fp);
      }
   }
   fclose(fp);

   this->umid = this->labelsize*this->ufeatures->getsize()-1;
   this->valid = true;
}

bool Cnftagger::check(char *tp)
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
         atof(tp);
         okay = true;
      }
   }

   return okay;
}

bool Cnftagger::tmplcheck()
{
   FILE *fp = NULL;
   if ((fp = fopen(this->tmpl.c_str(),"r")) == NULL)
   {
      fprintf (stderr,"Couldn't open %s\n",this->tmpl.c_str());
      exit (1);
   }
   int tmpl = 0;
   char buf[CNF_BUFSIZE];
   while (fgets(buf, CNF_BUFSIZE, fp) != NULL)
   {
      MyUtil::chomp(buf);
      if (MyUtil::IsCommentOut(buf))
      {
         continue;
      }
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

   return true;
}

char* Cnftagger::expand(char *tp, Sequence *s, int current)
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

void Cnftagger::storefset(Sequence *sq,
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

void Cnftagger::output(Sequence *s, std::vector<int>& labels)
{
   int row = s->getRowSize();
   std::vector<int>::reverse_iterator rit = labels.rbegin();
   for (int i = 0; i < row; i++, rit++)
   {
      for (int j = 0; j < (int)this->sqcolsize; j++)
      {
         fprintf (stdout,"%s ",s->getToken(i,j));
      }
      fprintf (stdout,"%s\n",this->label2surf[*rit].c_str());
   }
}

void Cnftagger::initlattice(vnode **lattice,
      std::vector<feature_t>& featureset)
{
   int col = featureset.size();
   int row = this->labelsize;
   for (int i = 0; i < col; i++)
   {
      for (int j = 0; j < row; j++)
      {
         lattice[i][j].id = j;
         lattice[i][j].cost = 0.;
         lattice[i][j].join = NULL;
         std::vector<int>::iterator uit = featureset[i].uf.begin();
         std::vector<int>::iterator utit = featureset[i].ut.begin();
         for (; uit != featureset[i].uf.end(); uit++, utit++)
         {
            int id = *uit;
            int tmpl = *utit;
            float w = *(this->model+id*row+j);
            if (this->fwit[tmpl] == this->fwit[tmpl+1])
            {
               lattice[i][j].cost += w;
            }
            else
            {
               float a = 0.;
               for (int k = this->fwit[tmpl]; k < this->fwit[tmpl+1]; k++)
               {
                  a += *(this->model+k);
               }
               lattice[i][j].cost += w * Cnflearn::logistic(a);
            }
         }
      }
   }
}

float Cnftagger::getbcost(int bias,
      int label,
      feature_t *featureset)
{
   int lsize = this->labelsize;
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

void Cnftagger::viterbi(Sequence *s,
      AllocMemdiscard *cache,
      std::vector<int>& lids)
{
   if (!this->valid)
   {
      fprintf (stderr,"model parameters is 0\n");
      exit (1);
   }
   /// store feature set
   std::vector<feature_t> featureset;
   this->storefset(s, featureset);

   /// build lattice
   int col = s->getRowSize();
   int row = this->labelsize;
   vnode **lattice = (vnode**)cache->alloc(sizeof(vnode*)*col);
   for (int c = 0; c < col; c++)
   {
      lattice[c] = (vnode*)cache->alloc(sizeof(vnode)*row);
   }
   this->initlattice(lattice,featureset);

   /// viterbi
   for (int i = 0; i < col; i++)
   {
      for (int j = 0; j < row; j++)
      {
         if (i == 0)
         {
            float cost = this->getbcost(0, j, &featureset[i]);
            lattice[i][j].cost += cost;
         }
         else
         {
            float max = 0.;
            int joinid = 0;
            for (int k = 0; k < row; k++)
            {
               float cost = lattice[i-1][k].cost
                  + this->getbcost(2*row+k*row, j, &featureset[i]);
               if (k == 0)
               {
                  max = cost;
               }
               if (cost > max)
               {
                  max = cost; joinid = k;
               }
            }
            lattice[i][j].join = &lattice[i-1][joinid];
            lattice[i][j].cost += max;
         }
      }
   }
   // eos
   feature_t e;
   if (this->bonly)
   {
      nodeptr *n = this->bfeatures->get("B");
      e.bf.push_back((*n)->val);
      e.bt.push_back(this->botmpl);
   }
   /*
      char *b = "B";
      int tmpl = this->f2t[b];
      nodeptr *n = this->bfeatures->get(b);
      nodeptr nil = this->bfeatures->getnil();
      feature_t e;
      if (*n != nil)
      {
      e.bf.push_back((*n)->val);
      e.bt.push_back(tmpl);
      }
    */
   float max = 0;
   int joinid = 0;
   for (int j = 0; j < row; j++)
   {
      float cost = lattice[col-1][j].cost
         + this->getbcost(row, j, &e);
      if (j == 0)
      {
         max = cost;
      }
      if (cost > max)
      {
         max = cost; joinid = j;
      }
   }

   /// backtrack
   vnode *bt = &lattice[col-1][joinid];
   for (; bt != NULL; bt = bt->join)
   {
      lids.push_back(bt->id);
   }
}

void Cnftagger::tagging(const char *corpus)
{
   if (corpus == NULL)
   {
      fprintf (stderr,"corpus is NULL\n");
      exit (1);
   }
   FILE *fp = NULL;
   if ((fp = fopen(corpus,"r")) == NULL)
   {
      fprintf (stderr,"Couldn't open %s\n",corpus);
      exit (1);
   }
   AllocMemdiscard cache(this->cachesize);
   Sequence sq;
   sq.setColSize(this->sqcolsize);
   sq.init();

   while (feof(fp) == 0)
   {
      MyUtil::sqread(fp, &sq, CNF_BUFSIZE);
      if (sq.getRowSize() == 0)
      {
         continue;
      }
      std::vector<int> lids;
      this->viterbi(&sq, &cache, lids);
      this->output(&sq, lids);
      sq.clear();
      cache.reset();
   }
   fclose(fp);
}
