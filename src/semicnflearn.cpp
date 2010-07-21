# include <semicnflearn.h>
# include <myutil.h>
# include <dic.h>
# include <sequence.h>
# include <allocmd.h>
# include <sparsevect.h>
# include <cstdlib>
# include <cmath>
# include <iostream>
# include <iomanip>
# include <ios>
# include <fstream>

# define SCNF_BUFSIZE 1024
# define SCNF_BLOCK 64

SemiCnflearn::SemiCnflearn(const char *tmpl, const char *corpus, unsigned int pool)
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
   this->fbound = 3;
   /// segment bound
   this->sbound = 1;
   /// lambda
   this->lambda = 1;
   /// alpha
   this->alpha = 1.1;
   /// alloc pool
   this->ac = new PoolAlloc(SCNF_BLOCK, pool);
   /// segment dic
   this->segments = new Dic(this->ac, CountUp);
   /// unigram segment dic
   this->usegments = new Dic(this->ac, Index);
   /// bigram segment dic
   this->bsegments = new Dic(this->ac, Index);
   /// feature dic
   this->features = new Dic(this->ac, CountUp);
   /// unigram feature dic
   this->ufeatures = new Dic(this->ac, Index);
   /// bigram feature dic
   this->bfeatures = new Dic(this->ac, Index);
   /// local label dic
   this->labels = new Dic(this->ac, Index);
   /// segment label dic
   this->slabels = new Dic(this->ac, Index);
   /// bonly flg
   this->bonly = false;
   /// tonly flg
   this->tonly = false;
   /// bonly tmpl id
   this->botmpl = -1;
   /// tonly tmpl id
   this->totmpl = -1;
   /// segment max length
   this->smaxlen = 0;
   /// segment template max length
   this->slen = 0;
   /// template check
   this->tmplcheck();
   /// feature extraction and encoding
   this->extfeature();
   /// cachesize
   this->cachesize = 1024*100000;
   /// coefficients
   this->c[0] = 0.0001; // unigram feature
   this->c[1] = 0.0001; // bigram feature
   this->c[2] = 0.0001; // unigram segment
   this->c[3] = 0.0001; // bigram segment
   this->c[4] = 0.0001; // gate function
}

SemiCnflearn::~SemiCnflearn()
{
   delete this->ufeatures;
   delete this->bfeatures;
   delete this->usegments;
   delete this->bsegments;
   delete this->slabels;
   delete this->labels;
   delete this->ac;
}

bool SemiCnflearn::init()
{
   /// feature rejection and making feature functions
   this->boundfeature();
   this->boundsegment();
   /// init model and pcache
   this->initmodel();
   /// first report
   this->report();
   this->valid = true;
   return this->valid;
}

void SemiCnflearn::decay(int t)
{
   double d = (double)t/this->instance;
   this->eta = this->lambda * std::pow((double)this->alpha, -d);

   // do c[i]/N * cc in reguralization process
   this->cc = this->eta;
}

void SemiCnflearn::storefset(Sequence *sq, std::vector<node_t>& lattice, AllocMemdiscard *cache)
{
   int size = sq->getRowSize();
   nodeptr unil = this->ufeatures->getnil();
   nodeptr bnil = this->bfeatures->getnil();
   nodeptr usnil = this->usegments->getnil();
   nodeptr bsnil = this->bsegments->getnil();
   for (int i = 0; i < size; ++i)
   {
      std::ifstream in(this->tmpl.c_str());
      std::string line;
      this->tmpli = 0;
      int ulen = 0;
      int blen = 0;
      std::vector<segments_t> us;
      std::vector<segments_t> bs;
      std::vector<int> butmpl; // begin id of unigram segment tmpl
      std::vector<int> bbtmpl; // begin id of bigram segment tmpl
      while (std::getline(in, line))
      {
         MyUtil::chomp(line);
         if (MyUtil::IsCommentOut(line.c_str()))
         {
            continue;
         }
         if (line[0] == 'U')
         {
            char *f = this->fexpand(line, sq, i);
            nodeptr *n = this->ufeatures->get(f);
            if (*n != unil)
            {
               lattice[i].tokenf.uf.push_back((*n)->val);
               lattice[i].tokenf.ut.push_back(this->tmpli);
            }
            this->ac->release(f);
            ++this->tmpli;
         }
         else if (line[0] == 'B')
         {
            char *f = this->fexpand(line, sq, i);
            nodeptr *n = this->bfeatures->get(f);
            if (*n != bnil)
            {
               lattice[i].tokenf.bf.push_back((*n)->val);
               lattice[i].tokenf.bt.push_back(this->tmpli);
            }
            this->ac->release(f);
            ++this->tmpli;
         }
         else if (line[0] == 'S')
         {
            ulen = this->sexpand(line, sq, i, us);
            butmpl.push_back(this->tmpli);
            this->tmpli+=ulen;
         }
         else if (line[0] == 'T')
         {
            blen = this->sexpand(line, sq, i, bs);
            bbtmpl.push_back(this->tmpli);
            this->tmpli+=blen;
         }
      }
      /// TODO: リファクタすべし
      /// make segments
      unsigned int uslen = 1;
      unsigned int bslen = 1;
      int ustmpl = 0;
      int bstmpl = 0;
      int lsize = this->slabelsize;
      while (!(uslen == us.size() && bslen == bs.size()))
      {
         if (
            ( (uslen < us.size() && us[uslen].size() == 0) && 
            (bslen < bs.size() && bs[bslen].size() == 0) ) || 
            ( uslen == us.size() &&
            (bslen < bs.size()) &&  bs[bslen].size() == 0) ||
            ( bslen == bs.size() &&
            (uslen < us.size() && us[uslen].size() == 0)))
         {
            break;
         }
         int ussize = us[uslen].size();
         int bssize = bs[0].size();
         if (bslen < bs.size())
            bssize += bs[bslen].size();
         segment_t s;
         s.sl = -1; // segment label
         s.bl = -1; // local begin label
         s.il = -1; // local inside label
         s.len = uslen;
         s._alpha = 0.; s._beta = 0.; s._lcost = 0.;
         s.us.id = (int*)cache->alloc(sizeof(int)*ussize);
         s.us.tid = (int*)cache->alloc(sizeof(int)*ussize);
         s.us.size = 0;
         s.bs.id = (int*)cache->alloc(sizeof(int)*bssize);
         s.bs.tid = (int*)cache->alloc(sizeof(int)*bssize);
         s.bs.size = 0;
         // tonly check
         if (bs[0].size() != 0)
         {
            if (std::strcmp(bs[0][0],"T") != 0)
            {
               std::cerr << "ERR: " << bs[0][0] << std::endl;
               exit(1);
            }
            nodeptr *n = this->bsegments->get("T");
            if (*n != bsnil)
            {
               s.bs.id[s.bs.size] = (*n)->val;
               s.bs.tid[s.bs.size++] = this->totmpl;
            }
         }
         if (uslen < bslen)
         {
            s.len = bslen;
            for (unsigned int j = 0; j < bs[bslen].size(); ++j)
            {
               nodeptr *n = this->bsegments->get(bs[bslen][j]);
               if (*n != bsnil)
               {
                  s.bs.id[s.bs.size] = (*n)->val;
                  s.bs.tid[s.bs.size++] = bbtmpl[j] + bstmpl;
               }
            }
         }
         else if (uslen > bslen)
         {
            for (unsigned int j = 0; j < us[uslen].size(); ++j)
            {
               nodeptr *n = this->usegments->get(us[uslen][j]);
               if (*n != usnil)
               {
                  s.us.id[s.us.size] = (*n)->val;
                  s.us.tid[s.us.size++] = butmpl[j] + ustmpl;
               }
            }
         }
         else
         {
            if (uslen < us.size())
               for (unsigned int j = 0; j < us[uslen].size(); ++j)
               {
                  nodeptr *n = NULL;
                  if (uslen < us.size())
                  {
                     n = this->usegments->get(us[uslen][j]);
                  }
                  if (n != NULL && *n != usnil)
                  {
                     s.us.id[s.us.size] = (*n)->val;
                     s.us.tid[s.us.size] = butmpl[j] + ustmpl;
                  }
               }
            if (bslen < bs.size())
               for (unsigned int j = 0; j < bs[bslen].size(); ++j)
               {
                  nodeptr *m = NULL;
                  if (bslen < bs.size())
                  {
                     m = this->bsegments->get(bs[bslen][j]);
                  }
                  if (m != NULL && *m != bsnil)
                  {
                     s.bs.id[s.bs.size] = (*m)->val;
                     s.bs.tid[s.bs.size] = bbtmpl[j] + bstmpl;
                  }
               }
         }
         if (uslen < us.size())
         {
            ++uslen;
            ++ustmpl;
         }
         if (bslen < bs.size())
         {
            ++bslen;
            ++bstmpl;
         }

         for (int l = 0; l < lsize; ++l)
         {
            segment_t *seg = (segment_t*)cache->alloc(sizeof(segment_t));
            *seg = s;
            seg->sl = l;
            seg->bl = this->sl2ll[l].bl;
            if (seg->len > 1)
            {
               seg->il = this->sl2ll[l].il;
            }
            else
            {
               seg->il = this->sl2ll[l].bl;
            }
            lattice[i].next.push_back(seg);
            if (i+seg->len <= size)
            {
               /*
                  std::cerr << "join"
                  << ' ' << seg->sl << ' ' << seg->uid
                  << ' ' << seg->len
                  << ' ' << i << "->" << i+seg->len
                  << std::endl;
                */
               lattice[i+seg->len].prev.push_back(seg);
            }
         }
      }
      // release
      for (unsigned int i = 0; i < us.size(); ++i)
      {
         std::vector<char*>::iterator it = us[i].begin();
         for (; it != us[i].end(); ++it)
         {
            this->ac->release(*it);
         }
      }
      for (unsigned int i = 0; i < bs.size(); ++i)
      {
         std::vector<char*>::iterator it = bs[i].begin();
         for (; it != bs[i].end(); ++it)
         {
            this->ac->release(*it);
         }
      }
   }
}

float SemiCnflearn::getfw(int id, int tmpl)
{
   float w = *(this->model+id);
   if (this->fwit[tmpl] == this->fwit[tmpl+1])
   {
      return w;
   }
   float a = 0;
   for (int k = this->fwit[tmpl]; k < this->fwit[tmpl+1]; ++k)
   {
      a += *(this->model+k);
   }
   return w*SemiCnflearn::logistic(a);
}

void SemiCnflearn::initlattice(std::vector<node_t>& lattice)
{
   std::vector<node_t>::iterator it = lattice.begin();
   int i = 0;
   for (; it != lattice.end(); ++it, ++i)
   {
      std::vector<segment_t*>::iterator sit = (*it).next.begin();
      for (; sit != (*it).next.end(); ++sit)
      {
         int len = (*sit)->len;
         int slabel = (*sit)->sl;
         /// unigram segment cost
         for (unsigned int ui = 0; ui < (*sit)->us.size; ++ui)
         {
            int usid = (*sit)->us.id[ui];
            int ustmpl = (*sit)->us.tid[ui];
            (*sit)->_lcost += this->getfw(this->bmid+1+usid*this->slabelsize+slabel, ustmpl);
         }
         /// token features cost
         for (int j = 0; j < len; ++j)
         {
            feature_t *t = &lattice[i+j].tokenf;
            /// unigram feature in segment
            findex_t ufit = t->uf.begin();
            findex_t utit = t->ut.begin();
            int llabel = -1;
            if (j == 0)
            {
               llabel = (*sit)->bl;
            }
            else
            {
               llabel = (*sit)->il;
            }
            for (; ufit != t->uf.end(); ++ufit, ++utit)
            {
               int id = *ufit;
               int tmpl = *utit;
               (*sit)->_lcost
                  += this->getfw(id*this->llabelsize+llabel, tmpl);
            }
            /// bigram feature in segment
            if (j == 0)
            {
               continue;
            }
            int bias = this->getbfbias(false, false, llabel);
            (*sit)->_lcost += this->getbfcost(t, bias, llabel);
         }
      }
   }
}

float SemiCnflearn::getbscost(int id, int tmpl, int label)
{
   return this->getfw(id+label, tmpl);
}

float SemiCnflearn::getbfcost(feature_t *f, int bias, int label)
{
   int b = this->umid+1+bias+label;
   float ret = 0.;
   findex_t bfit = f->bf.begin();
   findex_t btit = f->bt.begin();
   for (; bfit != f->bf.end(); ++bfit, ++btit)
   {
      int id = b + *bfit * (2*this->llabelsize+this->llabelsize*this->llabelsize);
      int tmpl = *btit;
      ret += this->getfw(id, tmpl);
   }

   return ret;
}

int SemiCnflearn::getbfbias(bool bos, bool eos, int label)
{
   if (bos)
   {
      return 0;
   }
   if (eos)
   {
      return this->llabelsize;
   }
   return 2*this->llabelsize+label*this->llabelsize;
}

int SemiCnflearn::getbsid(bool bos, bool eos, int bid, int label)
{
   int id = this->usid + 1 + bid * (2*this->slabelsize+this->slabelsize*this->slabelsize);
   if (bos)
   {
      return id;
   }
   if (eos)
   {
      return id+this->slabelsize;
   }
   return id + 2*this->slabelsize + label*this->slabelsize;
}

float SemiCnflearn::forward(std::vector<node_t>& lattice)
{
   int ebsid = -1;
   int ebstmpl = -1;
   if (this->tonly)
   {
      nodeptr *n = this->bsegments->get("T");
      ebsid = (*n)->val;
      ebstmpl = this->totmpl;
   }
   std::vector<node_t>::iterator it = lattice.begin();
   for (; it != lattice.end(); ++it)
   {
      std::vector<segment_t*>::iterator nit = (*it).next.begin();
      for (; nit != (*it).next.end(); ++nit)
      {
         if (it == lattice.begin())
         {
            float c = (*nit)->_lcost + this->getbfcost(&(*it).tokenf,
                  this->getbfbias((it==lattice.begin()),(it+1==lattice.end()),-1),
                  //(*nit)->l);
                  (*nit)->bl);
            for (unsigned int bi = 0; bi < (*nit)->bs.size; ++bi)
            {
               int id = this->getbsid((it==lattice.begin()),
                     (it+1==lattice.end()),
                     (*nit)->bs.id[bi],
                     -1);
               c += this->getbscost(id, (*nit)->bs.tid[bi], (*nit)->sl);
            }
            (*nit)->_alpha = SemiCnflearn::logsumexp((*nit)->_alpha, c, true);
         }
         else
         {
            std::vector<segment_t*>::iterator pit = (*it).prev.begin();
            for (; pit != (*it).prev.end(); ++pit)
            {
               float c = (*nit)->_lcost
                  + this->getbfcost(&(*it).tokenf,
                        this->getbfbias((it==lattice.begin()),(it+1==lattice.end()),(*pit)->il),
                        (*nit)->bl);
               for (unsigned int bi = 0; bi < (*nit)->bs.size; ++bi)
               {
                  int id = this->getbsid((it==lattice.begin()),
                        (it+1==lattice.end()),
                        (*nit)->bs.id[bi],
                        (*pit)->sl);
                  c += this->getbscost(id, (*nit)->bs.tid[bi], (*nit)->sl);
               }
               (*nit)->_alpha = SemiCnflearn::logsumexp((*nit)->_alpha,
                     (*pit)->_alpha+c,
                     (pit == (*it).prev.begin()));
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
   float z = 0.;
   --it; // alpha eos
   std::vector<segment_t*>::iterator pit = (*it).prev.begin();
   for (; pit != (*it).prev.end(); ++pit)
   {
      float c = this->getbfcost(&e,
            this->getbfbias((it==lattice.begin()),(it+1==lattice.end()),-1),
            (*pit)->il);
      if (this->tonly)
      {
         int id = this->getbsid((it==lattice.begin()),
               (it+1==lattice.end()),
               ebsid,
               -1);
         c += this->getbscost(id, ebstmpl, (*pit)->sl);
      }
      z = SemiCnflearn::logsumexp(z, (*pit)->_alpha+c, (pit == (*it).prev.begin()));
   }
   return z;
}

float SemiCnflearn::backward(std::vector<node_t>& lattice)
{
   feature_t e;
   if (this->bonly)
   {
      nodeptr *n = this->bfeatures->get("B");
      e.bf.push_back((*n)->val);
      e.bt.push_back(this->botmpl);
   }
   int ebsid = -1;
   int ebstmpl = -1;
   if (this->tonly)
   {
      nodeptr *n = this->bsegments->get("T");
      ebsid = (*n)->val;
      ebstmpl = this->totmpl;
   }
   std::vector<node_t>::reverse_iterator it = lattice.rbegin();
   for (; it != lattice.rend(); ++it)
   {
      std::vector<segment_t*>::iterator pit = (*it).prev.begin();
      for (; pit != (*it).prev.end(); ++pit)
      {
         if (it == lattice.rbegin())
         {
            float c = this->getbfcost(&e,
                  this->getbfbias((it+1==lattice.rend()),(it==lattice.rbegin()),-1),
                  (*pit)->il);
            if (this->tonly)
            {
               int id = this->getbsid((it+1==lattice.rend()),(it==lattice.rbegin()),ebsid,-1);
               c += this->getbscost(id, ebstmpl, (*pit)->sl);
            }
            (*pit)->_beta = SemiCnflearn::logsumexp((*pit)->_beta, c, true);
         }
         else
         {
            std::vector<segment_t*>::iterator nit = (*it).next.begin();
            for (; nit != (*it).next.end(); ++nit)
            {
               float c = (*nit)->_lcost
                  + this->getbfcost(&(*it).tokenf,
                        this->getbfbias((it+1==lattice.rend()),(it==lattice.rbegin()),(*pit)->il),
                        (*nit)->bl);
               for (unsigned int bi = 0; bi < (*nit)->bs.size; ++bi)
               {
                  int id = this->getbsid((it+1==lattice.rend()),
                        (it==lattice.rbegin()),
                        (*nit)->bs.id[bi],
                        (*pit)->sl);
                  c += this->getbscost(id, (*nit)->bs.tid[bi], (*nit)->sl);
               }
               (*pit)->_beta = SemiCnflearn::logsumexp((*pit)->_beta,
                     (*nit)->_beta+c,
                     (nit == (*it).next.begin()));
            }
         }
      }
   }

   float z = 0.;
   --it; // beta bos
   std::vector<segment_t*>::iterator nit = (*it).next.begin();
   for (; nit != (*it).next.end(); ++nit)
   {
      float c = (*nit)->_lcost
         + this->getbfcost(&(*it).tokenf,
               this->getbfbias((it+1==lattice.rend()),(it==lattice.rbegin()),-1),
               (*nit)->bl);
      for (unsigned int bi = 0; bi < (*nit)->bs.size; ++bi)
      {
         int id = this->getbsid((it+1==lattice.rend()),
               (it==lattice.rbegin()),
               (*nit)->bs.id[bi],-1);
         c += this->getbscost(id, (*nit)->bs.tid[bi], (*nit)->sl);
      }
      z = SemiCnflearn::logsumexp(z, (*nit)->_beta+c, (nit == (*it).next.begin()));
   }

   return z;
}

void SemiCnflearn::getclabels(Sequence *s, std::vector<correct_t>& cl)
{
   char *r = NULL;
   char *blabel = NULL;
   int row = s->getRowSize();
   //int p = -1;
   int c = 1;
   bool bflg = false;
   for (int i = 0; i < row; ++i)
   {
      r = s->getToken(i, this->labelcol);
      //nodeptr *l = this->labels->get(r);
      if (*r == 'B')
      {
         if (bflg)
         {
            char *slabel = this->cutbi(blabel);
            nodeptr *sl = this->slabels->get(slabel);
            correct_t n;
            n.sl = (*sl)->val;
            n.bl = this->sl2ll[(*sl)->val].bl;
            if (c > 1)
            {
               n.il = this->sl2ll[(*sl)->val].il;
            }
            else
            {
               n.il = this->sl2ll[(*sl)->val].bl;
            }
            n.len = c;
            cl.push_back(n);
         }
         blabel = r;
         bflg = true;
         c = 1;
      }
      else if (*r == 'I')
      {
         ++c;
      }
      else // O
      {
         if (bflg)
         {
            char *slabel = this->cutbi(blabel);
            nodeptr *sl = this->slabels->get(slabel);
            correct_t n;
            n.sl = (*sl)->val;
            n.bl = this->sl2ll[(*sl)->val].bl;
            if (c > 1)
            {
               n.il = this->sl2ll[(*sl)->val].il;
            }
            else
            {
               n.il = this->sl2ll[(*sl)->val].bl;
            }
            n.len = c;
            cl.push_back(n);
         }
         nodeptr *sl = this->slabels->get(r); // r == 'O'
         correct_t n;
         n.sl = (*sl)->val;
         n.bl = this->sl2ll[(*sl)->val].bl;
         n.il = this->sl2ll[(*sl)->val].il;
         n.len = 1;
         cl.push_back(n);
         bflg = false;
         blabel = NULL;
         c = 1;
      }
      /*
         if (p < 0)
         {
         p = (*l)->val;
         continue;
         }
         if (p == (*l)->val)
         {
         ++c;
         }
         else
         {
         correct_t n;
         n.l = p;
         n.len = c;
         cl.push_back(n);
         c= 1;
         }
         p = (*l)->val;
       */
   }
   if (bflg)
   {
      char *slabel = this->cutbi(blabel);
      nodeptr *sl = this->slabels->get(slabel);
      correct_t n;
      n.sl = (*sl)->val;
      n.bl = this->sl2ll[(*sl)->val].bl;
      if (c > 1)
      {
         n.il = this->sl2ll[(*sl)->val].il;
      }
      else
      {
         n.il = this->sl2ll[(*sl)->val].bl;
      }
      n.len = c;
      cl.push_back(n);
   }
   /*
      correct_t n;
      n.l = p;
      n.len = c;
      cl.push_back(n);
    */
}

void SemiCnflearn::upufweight(int label, feature_t *uf, SparseVector *v, float expect)
{
   findex_t uit = uf->uf.begin();
   findex_t utit = uf->ut.begin();
   for (; uit != uf->uf.end(); ++uit, ++utit)
   {
      int id = *uit * this->llabelsize + label;
      int tmpl = *utit;
      float a = 0;
      for (int k = this->fwit[tmpl]; k < this->fwit[tmpl+1]; ++k)
      {
         a += *(this->model+k);
      }
      float h = SemiCnflearn::logistic(a);
      v->add(id, h*expect);
      /// for gate function parameter
      float th = expect*h*(1.-h)* *(this->model+id);
      for (int k = this->fwit[tmpl]; k < this->fwit[tmpl+1]; ++k)
      {
         v->add(k, th);
      }
   }
}

void SemiCnflearn::upbfweight(int bias, int label, feature_t *bf, SparseVector *v, float expect)
{
   findex_t bit = bf->bf.begin();
   findex_t btit = bf->bt.begin();
   for (; bit != bf->bf.end(); ++bit, ++btit)
   {
      int id = this->umid + 1 + bias + label + *bit * (2*this->llabelsize+this->llabelsize*this->llabelsize);
      int tmpl = *btit;
      float w = *(this->model+id);
      if (this->fwit[tmpl] == this->fwit[tmpl+1])
      {
         v->add(id,expect);
      }
      else
      {
         float a = 0;
         for (int k = this->fwit[tmpl]; k < this->fwit[tmpl+1]; ++k)
         {
            a += *(this->model+k);
         }
         float h = SemiCnflearn::logistic(a);
         v->add(id,h*expect);
         /// for gate function parameter
         float th = expect*h*(1.-h)*w;
         for (int k = this->fwit[tmpl]; k < this->fwit[tmpl+1]; ++k)
         {
            v->add(k, th);
         }
      }
   }
}

void SemiCnflearn::upusweight(int label, sfeature_t *us, SparseVector *v, float expect)
{
   for (unsigned int i = 0; i < us->size; ++i)
   {
      int usid = us->id[i];
      int tmpl = us->tid[i];
      float a = 0;
      int id = this->bmid + 1 + usid * this->slabelsize + label;
      for (int k = this->fwit[tmpl]; k < this->fwit[tmpl+1]; ++k)
      {
         a += *(this->model+k);
      }
      float h = SemiCnflearn::logistic(a);
      v->add(id, h*expect);
      float th = expect*h*(1.-h)* *(this->model+id);
      for (int k = this->fwit[tmpl]; k < this->fwit[tmpl+1]; ++k)
      {
         v->add(k, th);
      }
   }
}

void SemiCnflearn::upbsweight(int id, int label, int tmpl, SparseVector *v, float expect)
{
   if (id < 0)
   {
      return;
   }
   float a = 0;
   id += label;
   for (int k = this->fwit[tmpl]; k < this->fwit[tmpl+1]; ++k)
   {
      a += *(this->model+k);
   }
   float h = SemiCnflearn::logistic(a);
   v->add(id, expect);
   float th = expect*h*(1.-h)* *(this->model+id);
   for (int k = this->fwit[tmpl]; k < this->fwit[tmpl+1]; ++k)
   {
      v->add(k, th);
   }
}

void SemiCnflearn::getcorrectv(std::vector<correct_t>& c,
      std::vector<node_t>& lattice,
      SparseVector *v)
{
   int ebsid = -1;
   int ebstmpl = -1;
   int t = 0;
   int plabel = -1;
   int plil = -1;
   if (this->tonly)
   {
      nodeptr *n = this->bsegments->get("T");
      ebsid = (*n)->val;
      ebstmpl = this->totmpl;
   }
   for (unsigned int p = 0; p < c.size(); ++p)
   {
      int label = c[p].sl;
      int lbl = c[p].bl;
      int lil = c[p].il;
      int len = c[p].len;
      for (int i = 0; i < len; ++i)
      {
         int bias = 0;
         if (i != 0)
         {
            this->upufweight(lil,&lattice[t+i].tokenf,v,1.);
            bias = this->getbfbias(false,false, lil);
            this->upbfweight(bias,lil,&lattice[t+i].tokenf,v,1.);
         }
         else
         {
            this->upufweight(lbl,&lattice[t+i].tokenf,v,1.);
            bias = this->getbfbias((plabel == -1),false, plil);
            this->upbfweight(bias,lbl,&lattice[t+i].tokenf,v,1.);
         }
      }
      int size = lattice[t].next.size();
      sfeature_t *us = NULL; sfeature_t *bs = NULL;
      int uplen = -1;
      for (int j = 0; j < size; ++j)
      {
         int slabel = lattice[p].next[j]->sl;
         //int sbl = lattice[p].next[j]->bl;
         //int sil = lattice[p].next[j]->il;
         int slen = lattice[p].next[j]->len;
         if (label == slabel && len == slen)
         {
            us = &lattice[t].next[j]->us;   bs = &lattice[t].next[j]->bs;
            /*
               std::cerr << "cblabel=" << this->label2surf[lbl]
               << " cilabel=" << this->label2surf[lil]
               << " sblabel=" << this->label2surf[sbl]
               << " silabel=" << this->label2surf[sil]
               << " uid=" << uid
               << " clen=" << len
               << " slen=" << slen
               << std::endl;
             */
            uplen = slen;
            break;
         }
      }
      //std::cerr << "up " << label << ' ' << uid << ' ' << uplen << std::endl;
      this->upusweight(label, us, v, 1.);
      for (unsigned int j = 0; j < bs->size; ++j)
      {
         int id = this->getbsid((plabel == -1), false, bs->id[j], plabel);
         this->upbsweight(id,label,bs->tid[j],v,1.);
      }
      t += len;
      plabel = label;
      plil = lil;
   }
   if (this->bonly)
   {
      feature_t e;
      nodeptr *n = this->bfeatures->get("B");
      e.bf.push_back((*n)->val);
      e.bt.push_back(this->botmpl);
      int bias = this->getbfbias(false, true, -1);
      //this->upbfweight(bias,plabel,&e,v,1.);
      this->upbfweight(bias,plil,&e,v,1.);
   }
   if (this->tonly)
   {
      int id = this->getbsid(false, true, ebsid, -1);
      this->upbsweight(id,plabel,ebstmpl,v,1.);
   }
}

void SemiCnflearn::getgradient(std::vector<node_t>& lattice, SparseVector *v, float z)
{
   int ebsid = -1;
   int ebstmpl = -1;
   if (this->tonly)
   {
      nodeptr *n = this->bsegments->get("T");
      ebsid = (*n)->val;
      ebstmpl = this->totmpl;
   }
   std::vector<node_t>::iterator it = lattice.begin();
   int i = 0;
   for (; it != lattice.end(); ++it, ++i)
   {
      std::vector<segment_t*>::iterator nit = (*it).next.begin();
      for (; nit != (*it).next.end(); ++nit)
      {
         float expect = 0.;
         int len = (*nit)->len;
         if (it == lattice.begin()) /// bos
         {
            int bias = this->getbfbias((it==lattice.begin()),(it+1==lattice.end()),-1);
            float c = (*nit)->_lcost
               + this->getbfcost(&(*it).tokenf,
                     bias,
                     (*nit)->bl);
            for (unsigned int j = 0; j < (*nit)->bs.size; ++j)
            {
               int id = -1;
               id = this->getbsid((it==lattice.begin()),
                     (it+1==lattice.end()),
                     (*nit)->bs.id[j],
                     -1);
               c += this->getbscost(id,
                     (*nit)->bs.tid[j],
                     (*nit)->sl);
            }
            float e = SemiCnflearn::myexp((*nit)->_beta + c - z);
            this->upbfweight(bias, (*nit)->bl, &(*it).tokenf, v, -e);
            for (unsigned int j = 0; j < (*nit)->bs.size; ++j)
            {
               int id = -1;
               id = this->getbsid((it==lattice.begin()),
                     (it+1==lattice.end()),
                     (*nit)->bs.id[j],
                     -1);
               this->upbsweight(id,(*nit)->sl,(*nit)->bs.tid[j],v,-e);
            }
            expect += e;
         }
         else
         {
            std::vector<segment_t*>::iterator pit = (*it).prev.begin();
            for (; pit != (*it).prev.end(); ++pit)
            {
               int bias = this->getbfbias((it==lattice.begin()),(it+1==lattice.end()),(*pit)->il);
               float c = (*nit)->_lcost
                  + this->getbfcost(&(*it).tokenf,
                        bias,
                        (*nit)->bl);
               for (unsigned int j = 0; j < (*nit)->bs.size; ++j)
               {
                  int id = -1;
                  id = this->getbsid((it==lattice.begin()),
                        (it+1==lattice.end()),
                        (*nit)->bs.id[j],
                        (*pit)->sl);
                  c += this->getbscost(id, (*nit)->bs.tid[j], (*nit)->sl);
               }
               float e = SemiCnflearn::myexp((*pit)->_alpha + (*nit)->_beta + c - z);
               /*
                  if ((*pit)->_alpha+(*nit)->_beta+c-z > 0.)
                  std::cerr << e
                  << ' '
                  << c
                  << ' '
                  << (*pit)->_alpha
                  << ' '
                  << (*nit)->_beta
                  << ' '
                  << z
                  << ' '
                  << (*pit)->_alpha+(*nit)->_beta+c-z
                  << ' '
                  << bias
                  << ' '
                  << this->getbfcost(&(*it).tokenf,bias,(*nit)->bl)
                  << std::endl;
                */
               //this->upbfweight(bias, (*nit)->l, &(*it).tokenf, v, -e);
               this->upbfweight(bias, (*nit)->bl, &(*it).tokenf, v, -e);
               for (unsigned int j = 0; j < (*nit)->bs.size; ++j)
               {
                  int id = -1;
                  id = this->getbsid((it==lattice.begin()),
                        (it+1==lattice.end()),
                        (*nit)->bs.id[j],
                        (*pit)->sl);
                  this->upbsweight(id,(*nit)->sl,(*nit)->bs.tid[j],v,-e);
               }
               expect += e;
            }
         }
         /*
            if (expect > 0.1)
            std::cerr << "p=" << i+1
            << " len=" << (*nit)->len
            << " blabel=" << this->label2surf[(*nit)->bl]
            << " ilabel=" << this->label2surf[(*nit)->il]
            << " slabel=" << (*nit)->sl
         //<< " uid=" << (*nit)->uid
         << " e=" << expect << std::endl;
          */
         /// token feature
         for (int j = 0; j < len; ++j)
         {
            /// unigram feature
            /// bigram feature
            if (j == 0)
            {
               /// unigram feature
               this->upufweight((*nit)->bl, &(*(it+j)).tokenf, v, -expect);
               continue;
            }
            /// unigram feature
            this->upufweight((*nit)->il, &(*(it+j)).tokenf, v, -expect);
            int bias = this->getbfbias(false,false,(*nit)->il);
            this->upbfweight(bias, (*nit)->il, &(*(it+j)).tokenf, v, -expect);
         }
         /// unigram segment
         this->upusweight((*nit)->sl, &(*nit)->us, v, -expect);
      }
   }
   /// eos
   --it;
   feature_t e;
   if (this->bonly)
   {
      nodeptr *n = this->bfeatures->get("B");
      e.bf.push_back((*n)->val);
      e.bt.push_back(this->botmpl);
   }
   std::vector<segment_t*>::iterator pit = (*it).prev.begin();
   for (; pit != (*it).prev.end(); ++pit)
   {
      int bias = this->getbfbias((it==lattice.begin()),(it+1==lattice.end()),-1);
      int id = -1;
      float c = this->getbfcost(&e,
            bias,
            (*pit)->il);
      if (this->tonly)
      {
         id = this->getbsid((it==lattice.begin()),
               (it+1==lattice.end()),
               ebsid,
               -1);
         c += this->getbscost(id, ebstmpl, (*pit)->sl);
      }
      float expect = SemiCnflearn::myexp((*pit)->_alpha + c - z);
      this->upbfweight(bias, (*pit)->il, &e, v, -expect);
      if (this->tonly)
      {
         this->upbsweight(id, (*pit)->sl, ebstmpl, v, -expect);
      }
   }
}

void SemiCnflearn::update(Sequence *s, AllocMemdiscard *cache, unsigned int reg)
{
   int col = s->getRowSize();
   /// build lattice
   std::vector<node_t> lattice(col+1);
   /// store feature and segment set
   this->storefset(s, lattice, cache);
   /// init lattice
   this->initlattice(lattice);
   /// get correct labels
   std::vector<correct_t> corrects;
   this->getclabels(s, corrects);
   /// get correct vector
   SparseVector grad(cache);
   this->getcorrectv(corrects, lattice, &grad);
   /// forward-backward
   float z1 = this->forward(lattice);
   float z2 = this->backward(lattice);

   //std::cerr << z1 << '-' << z2 << '=' << z1-z2 << std::endl;

   /// get expect vector
   this->getgradient(lattice, &grad, SemiCnflearn::max(z1,z2));

   /// update
   float err = 0.;
   std::list<int>::iterator it = grad.keys.begin();
   for (; it != grad.keys.end(); ++it)
   {
      float d = grad.get(*it);
      *(this->model+*it) += this->eta * d;
      err += d*d;
   }
   this->err += std::sqrt(err);
   /// regularization
   if (reg) /// L2
   {
      this->l2regularize(&grad);
   }
   else /// L1
   {
      this->l1regularize(&grad);
   }
}

void SemiCnflearn::l1regularize(SparseVector *v)
{
   float p = 0;
   std::list<int>::iterator it = v->keys.begin();
   for (; it != v->keys.end(); ++it)
   {
      p = this->pcache[*it]; // qi^{k-1}
      float w = *(this->model+*it); // w^{k+1/2}
      float uk = 0.;
      if (*it > (int)this->bsid)
      {
         uk = this->cc * this->c[4]/this->instance;
      }
      else if (*it > (int)this->usid)
      {
         uk = this->cc * this->c[3]/this->instance;
      }
      else if (*it > (int)this->bmid)
      {
         uk = this->cc * this->c[2]/this->instance;
      }
      else if (*it > (int)this->umid)
      {
         uk = this->cc * this->c[1]/this->instance;
      }
      else
      {
         uk = this->cc * this->c[0]/this->instance;
      }

      if (w > 0)
      {
         *(this->model+*it) = SemiCnflearn::max(0,w-(uk+p));
      }
      else if (w < 0)
      {
         *(this->model+*it) = SemiCnflearn::min(0,w+(uk-p));
      }
      this->pcache[*it] += *(this->model+*it) - w;
   }
}

void SemiCnflearn::l2regularize(SparseVector *v)
{
   float p = 0;
   std::list<int>::iterator it = v->keys.begin();
   for (; it != v->keys.end(); ++it)
   {
      float uk = 0.;
      if (*it > (int)this->bsid)
      {
         uk = this->cc * this->c[4]/this->instance;
      }
      else if (*it > (int)this->usid)
      {
         uk = this->cc * this->c[3]/this->instance;
      }
      else if (*it > (int)this->bmid)
      {
         uk = this->cc * this->c[2]/this->instance;
      }
      else if (*it > (int)this->umid)
      {
         uk = this->cc * this->c[1]/this->instance;
      }
      else
      {
         uk = this->cc * this->c[0]/this->instance;
      }
      p = uk - this->pcache[*it];
      this->pcache[*it] = uk;
      *(this->model+*it) /= (1.+p);
   }
}

void SemiCnflearn::learn(unsigned int iter, unsigned int reg)
{
   if (!this->valid)
   {
      std::cerr << "It's not initialized" << std::endl;
      exit(1);
   }
   AllocMemdiscard cache(this->cachesize);
   Sequence s;
   s.setColSize(this->sqcolsize);
   s.init();
   int t = 0;
   for (unsigned int i = 0; i < iter; ++i)
   {
      /// err
      this->err = 0.;
      std::ifstream in(this->corpus.c_str());
      std::string line;
      while (!in.eof())
      {
         MyUtil::sqread(in,&s);
         if (s.getRowSize() == 0)
         {
            continue;
         }
         this->decay(t++);
         this->update(&s, &cache, reg);
         s.clear();
         cache.reset();
      }
      this->lreport(i);
   }
}

void SemiCnflearn::lreport(unsigned int i)
{
   std::cerr << "epoch: " << i << ' '
      << "eta = " << this->eta << ' '
      << "err = " << this->err << std::endl;
}

void SemiCnflearn::report()
{
   std::cerr << "llabels: " << this->labels->getsize() << std::endl
      << "slabels: " << this->slabels->getsize() << std::endl
      << "fbound: " << this->fbound << std::endl
      << "sbound: " << this->sbound << std::endl
      << "ufeatures: " << this->ufeatures->getsize() << std::endl
      << "bfeatures: " << this->bfeatures->getsize() << std::endl
      << "usegments: " << this->usegments->getsize() << std::endl
      << "bsegments: " << this->bsegments->getsize() << std::endl
      << "instance: " << this->instance << std::endl
      << "gate functions: " << this->fwit.size()-1 << std::endl
      << "ufparameters: " << this->umid+1 << std::endl
      << "bfparameters: " << this->bmid-this->umid << std::endl
      << "usparameters: " << this->usid-this->bmid << std::endl
      << "bsparameters: " << this->bsid-this->usid << std::endl
      << "model parameters: " << this->parameters << std::endl;
}

void SemiCnflearn::initmodel()
{
   this->llabelsize = this->labels->getsize();
   this->slabelsize = this->slabels->getsize();
   int llabels = this->llabelsize;
   int slabels = this->slabelsize;
   int ufparams = this->ufeatures->getsize();
   int bfparams = this->bfeatures->getsize();
   int usparams = this->usegments->getsize();
   int bsparams = this->bsegments->getsize();
   int gparams = this->nk;
   int params = llabels * ufparams
      + (2*llabels+llabels*llabels)*bfparams
      + slabels*usparams
      + (2*slabels+slabels*slabels)*bsparams
      + gparams;
   this->umid = llabels*ufparams-1;
   this->bmid = this->umid
      + (2*llabels+llabels*llabels)*bfparams;
   this->usid = this->bmid
      + slabels*usparams;
   this->bsid = this->usid
      + (2*slabels+slabels*slabels)*bsparams;
   this->parameters = params;
   /**
    * model
    * +----------------------------------------------------+
    * | ufeature | bfeature | usegment | bsegment | g func |
    * +----------------------------------------------------+
    * ufeature, usegment
    * +----------------------------------------------------+
    * |l0,f0|l1,f0|l2,f0|...|ln,f0|l1,f1|l2,f1|......|ln,fn|
    * +----------------------------------------------------+
    * bfeature, bsegment
    * +------------------------------------------------------------------------+
    * |bos,l0,f0|..|bos,ln,f0|eos,l0,f0|..|eos,ln,f0|l0,l0,f0|...|ln,ln,f0|....|
    * +------------------------------------------------------------------------+
    */

   this->model = (float*)this->ac->alloc(sizeof(float)*params);
   this->pcache = (float*)this->ac->alloc(sizeof(float)*params);
   for (int i = 0; i < params-gparams; ++i)
   {
      *(this->model+i) = 0.;
      *(this->pcache+i) = 0.;
   }
   int t = 0;
   for (int j = params-gparams; j < params; ++j)
   {
      *(this->model+j) = this->theta[t++];
      *(this->pcache+j) = 0.;
   }
   int b = this->parameters - this->nk;
   for (int g = 0; g < (int)this->fwit.size(); ++g)
   {
      this->fwit[g] += b;
   }
}

void SemiCnflearn::storeff(nodeptr p, nodeptr nil)
{
   if (p == nil)
   {
      return;
   }
   this->storeff(p->left, nil);
   this->storeff(p->right, nil);
   if (p->val < (int)this->fbound)
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

void SemiCnflearn::storesf(nodeptr p, nodeptr nil)
{
   if (p == nil)
   {
      return;
   }
   this->storesf(p->left, nil);
   this->storesf(p->right, nil);
   if (p->val < (int)this->sbound)
   {
      return;
   }
   char *c = p->key;
   if (*c == 'S') // unigram segment
   {
      this->usegments->insert(c);
   }
   else if (*c == 'T') // bigram segment
   {
      this->bsegments->insert(c);
   }
   return;
}

void SemiCnflearn::boundfeature()
{
   nodeptr nil = this->features->getnil();
   for (int i = 0; i < HASHSIZE; ++i)
   {
      nodeptr *p = (this->features->table+i);
      if (*p == nil)
      {
         continue;
      }
      this->storeff(*p,nil);
   }
   delete this->features;
}

void SemiCnflearn::boundsegment()
{
   nodeptr nil = this->segments->getnil();
   for (int i = 0; i < HASHSIZE; ++i)
   {
      nodeptr *p = (this->segments->table+i);
      if (*p == nil)
      {
         continue;
      }
      this->storesf(*p,nil);
   }
   delete this->segments;
}

char* SemiCnflearn::fexpand(std::string& t, Sequence *s, int current)
{
   char b[t.size()];
   std::strcpy(b,t.c_str());
   char *tp = b;

   std::string f = "";
   for (char *p = tp; *p != '\0'; ++p)
   {
      if (*p == '%' && *(p+1) == 'x' && *(p+2) == '[')
      {
         *p++ = '\0';
         f += tp;
         tp = p+2;
         for (; *p != ','; ++p) ;
         *p++ = '\0';
         int row = atoi(tp);
         tp = p;
         for (; *p != ','; ++p) ;
         *p++ = '\0';
         int col = atoi(tp);
         tp = p;
         for (; *p != ']'; ++p) ;
         *p = '\0';
         f += s->getToken(current+row,col);
         tp = p+1;
      }
   }
   if (*tp != '\0')
   {
      f += tp;
   }
   char *feature = (char*)this->ac->alloc(sizeof(char*)*f.size()+1);
   std::strcpy(feature,f.c_str());
   return feature;
}

int SemiCnflearn::sexpand(std::string& t, Sequence *s, int current, std::vector<segments_t>& segments)
{
   char b[t.size()];
   std::strcpy(b,t.c_str());
   char *tp = b;
   std::string prefix = "";
   std::string suffix = "";

   int len = 0;
   int size = s->getRowSize();
   for (char *p = tp; *p != '\0'; ++p)
   {
      if (*p == '%' && *(p+1) == 's' && *(p+2) == '[')
      {
         *p++ = '\0';
         prefix += tp;
         tp = p+2;
         for (; *p != ','; ++p) ;
         *p++ = '\0';
         len = atoi(tp);
         if (segments.size() < (unsigned int)len+1)
         {
            segments.resize(len+1);
         }
         tp = p;
         for (; *p != ','; ++p) ;
         *p++ = '\0';
         int col = atoi(tp);
         for (; *p != ']'; ++p) ;
         *p = '\0';
         tp = p+1;
         suffix += tp;
         /// generate segments
         std::string str = prefix;
         for (int l = 1; l <= len; ++l)
         {
            if (current+l > size)
            {
               continue;
            }
            if (l-1 > 0)
            {
               str += "/";
            }
            str += s->getToken(current+l-1,col);
            if (str.size() > 0)
            {
               char *segment = (char*)this->ac->alloc(sizeof(char*)*str.size() + sizeof(char*)*suffix.size());
               std::strcpy(segment, str.c_str());
               std::strcat(segment, suffix.c_str());
               segments[l].push_back(segment);
            }
         }
      }
   }
   if (len == 0 && *tp != '\0') /// tonly
   {
      prefix += tp;
      char *segment = (char*)this->ac->alloc(sizeof(char*)*prefix.size());
      std::strcpy(segment,prefix.c_str());
      if (segments.size() == 0)
      {
         segments.resize(1);
      }
      segments[0].push_back(segment);
   }
   return len;
}

void SemiCnflearn::extract(Sequence *s)
{
   int size = s->getRowSize();
   for (int i = 0; i < size; ++i)
   {
      std::ifstream in(this->tmpl.c_str());
      std::string line;
      while (std::getline(in,line))
      {
         MyUtil::chomp(line);
         if (MyUtil::IsCommentOut(line.c_str()))
         {
            continue;
         }
         if (line[0] == 'B' || line[0] == 'U') /// feature
         {
            char *f = this->fexpand(line, s, i);
            this->features->insert(f);
            this->ac->release(f);
         }
         else if (line[0] == 'S' || line[0] == 'T') /// segment
         {
            std::vector<segments_t> seg;
            int len = this->sexpand(line, s, i, seg);
            for (int j = 0; j <= len; ++j)
            {
               std::vector<char*>::iterator it = seg[j].begin();
               for (; it != seg[j].end(); ++it)
               {
                  this->segments->insert(*it);
                  this->ac->release(*it);
               }
            }
         }
      }
   }
}

char* SemiCnflearn::cutbi(char *l)
{
   char *head = l;
   for (; *head != '\0' && *head != '-'; ++head) ;

   if (*head != '\0')
   {
      return head+1;
   }
   return l;
}

void SemiCnflearn::mapping(char *l, int id)
{
   nodeptr nil = this->labels->getnil();
   int bi = -1; int ii = -1;
   if (std::strcmp(l,"O") == 0)
   {
      nodeptr *on = this->labels->get(l);
      int bi = (*on)->val; int ii = (*on)->val;
      llabel_t local;
      local.bl = bi; local.il = ii;
      this->sl2ll[id] = local;
      return;
   }
   // mapping segment label to 
   // begin of local label and inside of local label
   std::string b = "B-"; b += l;
   std::string i = "I-"; i += l;
   nodeptr *bn = this->labels->get(b.c_str());
   nodeptr *in = this->labels->get(i.c_str());
   if (*bn != nil)
   {
      bi = (*bn)->val;
   }
   else
   {
      char buf[b.size()];
      std::strcpy(buf,b.c_str());
      nodeptr n = this->labels->insert(buf);
      if (n == NULL)
      {
         std::cerr << "ERR: Dic insert failed ("
            << b << ')'
            << std::endl;
         exit(1);
      }
      this->label2surf.push_back(n->key);
      bi = n->val;
   }
   if (*in != nil)
   {
      ii = (*in)->val;
   }
   else
   {
      char buf[i.size()];
      std::strcpy(buf,i.c_str());
      nodeptr n = this->labels->insert(buf);
      if (n == NULL)
      {
         std::cerr << "ERR: Dic insert failed ("
            << i << ')'
            << std::endl;
         exit(1);
      }
      this->label2surf.push_back(n->key);
      ii = n->val;
   }
   llabel_t local;
   local.bl = bi; local.il =ii;
   this->sl2ll[id] = local;
}

void SemiCnflearn::extlabel(Sequence *s)
{
   //char *p = NULL;
   char *r = NULL;
   int c = 1;
   int row = s->getRowSize();
   bool bflg = false;
   for (int i = 0; i < row; ++i)
   {
      r = s->getToken(i, this->labelcol);
      nodeptr l = this->labels->insert(r);
      if (l != NULL)
      {
         this->label2surf.push_back(l->key);
      }
      char *slabel = this->cutbi(r);
      nodeptr sl = this->slabels->insert(slabel);
      if (sl != NULL)
      {
         // mapping segment label to 
         // begin of local label and inside of local label
         this->mapping(sl->key, sl->val);
      }
      if (*r == 'B')
      {
         if (bflg)
         {
            if (c > this->smaxlen)
            {
               this->smaxlen = c;
            }
         }
         bflg = true;
         c = 1;
      }
      else if (*r == 'I')
      {
         if (!bflg)
         {
            std::cerr << "WARNING: Found unexpected labeling" << std::endl;
            s->dump();
            exit(1);
         }
         ++c;
      }
      else /// O label
      {
         bflg = false;
         c = 1;
      }
   }
}

void SemiCnflearn::extfeature()
{
   std::ifstream in(this->corpus.c_str());
   std::string line;
   Sequence s;
   s.setColSize(this->sqcolsize);
   s.init();
   this->instance = 0;
   while (!in.eof())
   {
      MyUtil::sqread(in,&s);
      if (s.getRowSize() == 0)
      {
         continue;
      }
      this->extlabel(&s);
      this->extract(&s);
      ++this->instance;
      s.clear();
   }
   if (this->smaxlen > this->slen)
   {
      std::cerr << "WARNING: "
         << "Segment maxlength in template "
         << this->slen
         << " < Segment maxlength in corpus "
         << this->smaxlen
         << std::endl;
      exit(1);
   }
}

bool SemiCnflearn::check(std::string& t)
{
   char b[t.size()];
   std::strcpy(b,t.c_str());
   char *tp = b;

   if (*tp != 'U' && *tp != 'B' && *tp != 'S' && *tp != 'T')
   {
      return false;
   }
   bool segment = false;
   bool feature = false;
   bool duplication = false;
   bool okay = true;
   for (char *p = tp; *p != '\0'; ++p)
   {
      if (*p == '%' && *(p+1) == 'x' && *(p+2) == '[')
      {
         okay = false;
         *p++ = '\0';
         tp = p+2;
         for (; *p != ',' && *p != '\0'; ++p) ;
         if (*p == '\0')
         {
            break;
         }
         *p++ = '\0';
         atoi(tp); // row
         tp = p;
         for (; *p != ',' && *p != '\0'; ++p) ;
         if (*p == '\0')
         {
            break;
         }
         *p++ = '\0';
         atoi(tp); // col
         tp = p;
         for (; *p != ']' && *p != '\0'; ++p) ;
         if (*p == '\0')
         {
            break;
         }
         *p = '\0';
         float t= atof(tp); // weight
         // count up number of weights
         ++this->nk;
         // init theta parameter
         this->theta.push_back(t);
         okay = true;
         feature = true;
      }
      else if (*p == '%' && *(p+1) == 's' && *(p+2) == '[')
      {
         if (segment)
         {
            duplication = true;
         }
         okay = false;
         *p++ = '\0';
         tp = p+2;
         for (; *p != ',' && *p != '\0'; ++p) ;
         if (*p == '\0')
         {
            break;
         }
         *p++ = '\0';
         int len = atoi(tp); // max length
         tp = p;
         for (; *p != ',' && *p != '\0'; ++p) ;
         if (*p == '\0')
         {
            break;
         }
         *p++ = '\0';
         atoi(tp); // col
         tp = p;
         for (; *p != ']' && *p != '\0'; ++p) ;
         if (*p == '\0')
         {
            break;
         }
         *p++ = '\0';
         float t = atof(tp); // weight
         // expand template
         this->tmpli+=len;
         for (int i = 1; i < len; ++i)
         {
            // count up number of weight
            ++this->nk;
            // init theta
            this->theta.push_back(t);
            this->fwit.push_back(this->nk);
         }
         ++this->nk;
         okay = true;
         segment = true;
         if (len > this->slen)
         {
            this->slen = len;
         }
      }
   }
   if (feature & segment)
   {
      okay = false;
   }
   if (duplication)
   {
      okay = false;
   }
   return okay;
}

bool SemiCnflearn::tmplcheck()
{
   this->nk = 0;
   this->tmpli = 0;
   std::ifstream in(this->tmpl.c_str());
   std::string line;
   while (std::getline(in,line))
   {
      MyUtil::chomp(line);
      if (MyUtil::IsCommentOut(line.c_str()))
      {
         continue;
      }
      this->fwit.push_back(this->nk);
      if (!this->check(line))
      {
         std::cerr << "ERR: Invalid template "
            << line << std::endl;
         exit(1);
      }
      if (std::strcmp(line.c_str(),"B") == 0)
      {
         this->bonly = true;
         this->botmpl = this->tmpli;
      }
      if (std::strcmp(line.c_str(),"T") == 0)
      {
         this->tonly = true;
         this->totmpl = this->tmpli++;
      }
      if (line[0] == 'B' || line[0] == 'U')
      {
         ++this->tmpli;
      }
   }
   this->fwit.push_back(this->nk);
   return true;
}

void SemiCnflearn::dumpllabels(std::ofstream& out)
{
   out << "Start_lLabel" << std::endl;
   for (unsigned int i = 0; i < this->label2surf.size(); ++i)
   {
      out << '[' << i << "] " << this->label2surf[i] << std::endl;
   }
   out << "End_lLabel" << std::endl;
}

void SemiCnflearn::dumpsl2ll(std::ofstream& out)
{
   out << "Start_sl2ll" << std::endl;
   for (unsigned int i = 0; i < this->sl2ll.size(); ++i)
   {
      out << '[' << i << "] " << this->sl2ll[i].bl << ' ' << this->sl2ll[i].il << std::endl;
   }
   out << "End_sl2ll" << std::endl;
}

void SemiCnflearn::dumpfwit(std::ofstream& out)
{
   out << "Start_Fwit" << std::endl;
   for (unsigned int i = 0; i < this->fwit.size(); ++i)
   {
      out << '[' << i << "] " << this->fwit[i] << std::endl;
   }
   out << "End_Fwit" << std::endl;
}

void SemiCnflearn::dumpparams(std::ofstream& out)
{
   out << "Start_Params" << std::endl;
   out.setf(std::ios::scientific);
   for (unsigned int i = 0; i < this->parameters; ++i)
   {
      out << '[' << i << "] " << *(this->model+i) << std::endl;
   }
   out.unsetf(std::ios::scientific);
   out << "End_Params" << std::endl;
}

void SemiCnflearn::inversef(nodeptr p, nodeptr nil, std::vector<char*>& f)
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

void SemiCnflearn::callinv(nodeptr *p, nodeptr nil, std::vector<char*>& f)
{
   for (int i = 0; i < HASHSIZE; ++i)
   {
      nodeptr *q = (p+i);
      if (*q == nil)
      {
         continue;
      }
      this->inversef(*q,nil,f);
   }
}

void SemiCnflearn::dumpufeatures(std::ofstream& out, std::vector<char*>& f)
{
   out << "Start_uFeatures" << std::endl;
   for (unsigned int i = 0; i < f.size(); ++i)
   {
      out << '[' << i << "] " << f[i] << std::endl;
   }
   out << "End_uFeatures" << std::endl;
}

void SemiCnflearn::dumpbfeatures(std::ofstream& out, std::vector<char*>& f)
{
   out << "Start_bFeatures" << std::endl;
   for (unsigned int i = 0; i < f.size(); ++i)
   {
      out << '[' << i << "] " << f[i] << std::endl;
   }
   out << "End_bFeatures" << std::endl;
}

void SemiCnflearn::dumpusegments(std::ofstream& out, std::vector<char*>& f)
{
   out << "Start_uSegments" << std::endl;
   for (unsigned int i = 0; i < f.size(); ++i)
   {
      out << '[' << i << "] " << f[i] << std::endl;
   }
   out << "End_uSegments" << std::endl;
}

void SemiCnflearn::dumpbsegments(std::ofstream& out, std::vector<char*>& f)
{
   out << "Start_bSegments" << std::endl;
   for (unsigned int i = 0; i < f.size(); ++i)
   {
      out << '[' << i << "] " << f[i] << std::endl;
   }
   out << "End_bSegments" << std::endl;
}

void SemiCnflearn::dumpfeatures(std::ofstream& out)
{
   nodeptr ufnil = this->ufeatures->getnil();
   nodeptr bfnil = this->bfeatures->getnil();
   nodeptr usnil = this->usegments->getnil();
   nodeptr bsnil = this->bsegments->getnil();

   std::vector<char*> ufs(this->ufeatures->getsize());
   this->callinv(this->ufeatures->table, ufnil, ufs);
   this->dumpufeatures(out, ufs);
   ufs.clear();

   std::vector<char*> bfs(this->bfeatures->getsize());
   this->callinv(this->bfeatures->table, bfnil, bfs);
   this->dumpbfeatures(out, bfs);
   bfs.clear();

   std::vector<char*> uss(this->usegments->getsize());
   this->callinv(this->usegments->table, usnil, uss);
   this->dumpusegments(out, uss);
   ufs.clear();

   std::vector<char*> bss(this->bsegments->getsize());
   this->callinv(this->bsegments->table, bsnil, bss);
   this->dumpbsegments(out, bss);
   bfs.clear();
}

void SemiCnflearn::save(const char *save)
{
   std::ofstream out(save);
   out << "Params " << this->parameters << std::endl;
   out << "lLabels " << this->llabelsize << std::endl;
   out << "sLabels " << this->slabelsize << std::endl;

   /// local label id to surface
   this->dumpllabels(out);

   /// segment label to local labels
   this->dumpsl2ll(out);

   /// fwit
   this->dumpfwit(out);

   /// params
   this->dumpparams(out);

   /// features
   this->dumpfeatures(out);
}
