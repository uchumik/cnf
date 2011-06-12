# include "rtcrftagger.hpp"
# define CRFTAGGER_BUFSIZE 4096

using namespace RtCrf;
using namespace sequential;
using namespace toyutil;
using namespace std;

Crftagger::Crftagger(const char *tmpl, unsigned int poolsize)
: Crf(tmpl, poolsize)
{
   this->delimit = " ";
}

Crftagger::~Crftagger()
{
}

void Crftagger::read(const char *model)
{
   if (this->valid)
   {
      throw "Model is not cleared";
   }
   FILE *fp = NULL;
   if ((fp = fopen(model, "rb")) == NULL)
   {
      throw "Couldn't open modelfile";
   }

   char buf[CRFTAGGER_BUFSIZE];
   while (fgets(buf, CRFTAGGER_BUFSIZE, fp) != NULL)
   {
      MyUtil::chomp(buf);
      if (MyUtil::IsCommentOut(buf))
      {
         continue;
      }
      if (strncmp(buf,"Params=",7) == 0)
      {
         int params = 0;
         sscanf (buf+7,"%d",&params);
         this->model = (float*)this->ac->alloc(sizeof(float)*params);
         this->parameters = params;
      }
      else if (strncmp(buf,"Labels=",7) == 0)
      {
         sscanf(buf+7,"%d",&this->labelsize);
         this->label2surf.resize(this->labelsize);
      }
      else if (strncmp(buf,"Start_Label",11) == 0)
      {
         this->setlabel(fp);
      }
      else if (strncmp(buf,"Start_Params",12) == 0)
      {
         this->setparams(fp);
      }
      else if (strncmp(buf,"Start_uFeatures",15) == 0)
      {
         this->setufeatures(fp);
      }
      else if (strncmp(buf,"Start_bFeatures",15) == 0)
      {
         this->setbfeatures(fp);
      }
   }
   fclose(fp);
   this->umid = this->labelsize*this->ufeatures->getsize()-1;
   this->valid = true;
}

void Crftagger::setlabel(FILE *fp)
{
   char buf[CRFTAGGER_BUFSIZE];
   while (fgets(buf, CRFTAGGER_BUFSIZE, fp) != NULL)
   {
      MyUtil::chomp(buf);
      if (MyUtil::IsCommentOut(buf))
      {
         continue;
      }
      if (std::strncmp(buf,"End_Label",9) == 0)
      {
         break;
      }
      char l[CRFTAGGER_BUFSIZE];
      int id = -1;
      sscanf(buf,"[%d]=%s",&id,l);
      if (id < 0)
      {
         string err = "Found unknown parameter: ";
         err += buf;
         throw err.c_str();
      }
      this->label2surf[id] = l;
   }
}

void Crftagger::setparams(FILE *fp)
{
   fread(this->model,1,sizeof(float)*this->parameters,fp);
   char buf[CRFTAGGER_BUFSIZE];
   while (fgets(buf, CRFTAGGER_BUFSIZE, fp) != NULL)
   {
      MyUtil::chomp(buf);
      if (MyUtil::IsCommentOut(buf))
      {
         continue;
      }
      if (strncmp(buf,"End_Params",10) == 0)
      {
         break;
      }
   }
}

void Crftagger::setufeatures(FILE *fp)
{
   char buf[CRFTAGGER_BUFSIZE];
   while (fgets(buf, CRFTAGGER_BUFSIZE, fp) != NULL)
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
      char f[CRFTAGGER_BUFSIZE];
      sscanf(buf,"[%d]=%s",&id,f);
      if (id < 0)
      {
         string err = "Unknown parameter: ";
         err += buf;
         throw err.c_str();
      }
      nodeptr n = this->ufeatures->insert(f);
      if (n->val != id)
      {
         string err = "Unknown parameter: ";
         err += buf;
         throw err.c_str();
      }
   }
}

void Crftagger::setbfeatures(FILE *fp)
{
   char buf[CRFTAGGER_BUFSIZE];
   while (fgets(buf, CRFTAGGER_BUFSIZE, fp) != NULL)
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
      char f[CRFTAGGER_BUFSIZE];
      sscanf(buf,"[%d]=%s",&id,f);
      if (id < 0)
      {
         string err = "Unknown parameter: ";
         err += buf;
         throw err.c_str();
      }
      nodeptr n = this->bfeatures->insert(f);
      if (n->val != id)
      {
         string err = "Unknown parameter: ";
         err += buf;
         throw err.c_str();
      }
   }
}

/*
float Crftagger::getbcost(int bias,
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

void Crftagger::initlattice(vnode **lattice,
      std::vector<feature_t>& featureset)
{
   int col = featureset.size();
   int row = this->labelsize;
   for (int i = 0; i < col; ++i)
   {
      for (int j = 0; j < row; ++j)
      {
         lattice[i][j].id = j;
         lattice[i][j].cost = 0.;
         lattice[i][j].join = NULL;
         std::vector<pair<int,double> >::iterator uit = featureset[i].uf.begin();
         for (; uit != featureset[i].uf.end(); ++uit)
         {
            int id = (*uit).first;
            float w = *(this->model+id*row+j);
            lattice[i][j].cost += w * (*uit).second;
         }
      }
   }
}

void Crftagger::output(Sequence *s, std::vector<int>& labels)
{
   int row = s->getRowSize();
   std::vector<int>::reverse_iterator rit = labels.rbegin();
   for (int i = 0; i < row; ++i, ++rit)
   {
      for (int j = 0; j < (int)this->sqcolsize; ++j)
      {
         fprintf (stdout,"%s ",s->getToken(i,j));
      }
      fprintf (stdout,"%s\n",this->label2surf[*rit].c_str());
   }
}

void Crftagger::viterbi(Sequence *s,
      AllocMemdiscard *cache,
      vector<int>& lids)
{
   if (!this->valid)
   {
      throw "modelparameter didn't get initialized";
   }
   /// store feature set
   fsetv_t fv;
   vector<feature_t> featureset;
   this->storefset(fv, featureset, s);
   //fsetvdump(fv);

   /// build lattice
   int col = s->getRowSize();
   int row = this->labelsize;
   vnode **lattice = (vnode**)cache->alloc(sizeof(vnode*)*col);
   for (int c = 0; c < col; ++c)
   {
      lattice[c] = (vnode*)cache->alloc(sizeof(vnode)*row);
   }
   this->initlattice(lattice,featureset);

   /// viterbi
   for (int i = 0; i < col; ++i)
   {
      for (int j = 0; j < row; ++j)
      {
         if (i == 0)
         {
            float cost = this->getbcost(0,j,&featureset[i]);
            lattice[i][j].cost += cost;
         }
         else
         {
            float max = 0.;
            int joinid = 0;
            for (int k = 0; k < row; ++k)
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
      pair<int, double> f;
      nodeptr *n = this->bfeatures->get("B");
      f.first = (*n)->val;
      f.second = 1.0;
      e.bf.push_back(f);
   }
   float max = 0;
   int joinid = 0;
   for (int j = 0; j < row; ++j)
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

void Crftagger::tagging(const char *corpus)
{
   if (corpus == NULL)
   {
      string err = "corpus is not exist";
      throw err.c_str();
   }
   this->sqcolsize = this->checkcolsize(corpus,this->delimit.c_str());
   FILE *fp = NULL;
   if ((fp = fopen(corpus,"r")) == NULL)
   {
      throw "Couldn't open corpus";
   }
   AllocMemdiscard cache(this->cachesize);
   Sequence sq;
   sq.setColSize(this->sqcolsize);
   sq.setArraySize(this->sqarraysize);
   sq.setDelimit(this->delimit.c_str());
   sq.init();

   while (feof(fp) == 0)
   {
      MyUtil::sqread(fp, &sq, CRFTAGGER_BUFSIZE);
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
