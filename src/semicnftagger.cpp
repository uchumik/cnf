# include <semicnftagger.h>
# include <algorithm>
# include <iostream>
# include <sstream>
# include <string>

# define SCNF_BUFSIZE 1024
# define SCNF_BLOCK 128

SemiCnftagger::SemiCnftagger(const char *tmpl, unsigned int pool)
{
   this->tmpl = tmpl;
   /// sqcolsize
   this->sqcolsize = 3;
   /// alloc pool
   this->ac = new PoolAlloc(SCNF_BLOCK, pool);
   /// unigram feature dic
   this->ufeatures = new Dic(this->ac, Index);
   /// bigram feature dic
   this->bfeatures = new Dic(this->ac, Index);
   /// unigram segment dic
   this->usegments = new Dic(this->ac, Index);
   /// bigram segment dic
   this->bsegments = new Dic(this->ac, Index);
   /// bonly flg
   this->bonly = false;
   /// tonly flg
   this->tonly = false;
   /// template check
   this->tmplcheck();
   /// cache size
   this->cachesize = 1024*100000;
   this->valid = false;
}

SemiCnftagger::~SemiCnftagger()
{
   if (this->valid)
   {
      this->ac->release(this->model);
   }
   delete this->ufeatures;
   delete this->bfeatures;
   delete this->usegments;
   delete this->bsegments;
   delete this->ac;
}


bool SemiCnftagger::check(std::string& t)
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
         atof(tp); // weight
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
         for (; *p != ']' && *p != '\0'; ++p) ;
         if (*p == '\0')
         {
            break;
         }
         *p++ = '\0';
         float t = atof(tp); // weight
         // expand template
         this->tmpli+=len;
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

bool SemiCnftagger::tmplcheck()
{
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
   return true;
}

void SemiCnftagger::readparamsize(std::string& line)
{
   std::istringstream is(line);
   std::string c;
   int size = 0;
   is >> c >> size;

   if (size <= 0)
   {
      std::cerr << "ERR: " << line << std::endl;
      return;
   }
   this->model = (float*)this->ac->alloc(sizeof(float)*size);
}

void SemiCnftagger::readllabelsize(std::string& line)
{
   std::istringstream is(line);
   std::string c;
   int size = 0;
   is >> c >> size;

   if (size <= 0)
   {
      std::cerr << "ERR: " << line << std::endl;
      return;
   }
   this->llabelsize = size;
   this->label2surf.resize(this->llabelsize);
}

void SemiCnftagger::readslabelsize(std::string& line)
{
   std::istringstream is(line);
   std::string c;
   int size = 0;
   is >> c >> size;

   if (size <= 0)
   {
      std::cerr << "ERR: " << line << std::endl;
      return;
   }
   this->slabelsize = size;
}

void SemiCnftagger::readllabels(std::ifstream& in)
{
   std::string line;
   while (std::getline(in, line))
   {
      MyUtil::chomp(line);
      if (MyUtil::IsCommentOut(line.c_str()))
      {
         continue;
      }
      if (line == "End_lLabel")
      {
         break;
      }
      char l, r;
      std::string label;
      int i = -1;// label id
      std::istringstream is(line);
      is >> l >> i >> r >> label;
      if (i < 0)
      {
         std::cerr << "ERR: " << line << std::endl;
         exit(1);
      }
      this->label2surf[i] = label;
   }
}

void SemiCnftagger::readmap(std::ifstream& in)
{
   std::string line;
   while (std::getline(in, line))
   {
      MyUtil::chomp(line);
      if (MyUtil::IsCommentOut(line.c_str()))
      {
         continue;
      }
      if (line == "End_sl2ll")
      {
         break;
      }
      char l,r;
      int i = -1, bl = -1, il = -1;
      std::istringstream is(line);
      is >> l >> i >> r >> bl >> il;
      if (i < 0 || bl < 0 || il < 0)
      {
         std::cerr << "ERR: " << line << std::endl;
         exit(1);
      }
      llabel_t ll;
      ll.bl = bl; ll.il = il;
      this->sl2ll[i] = ll;
   }
}

void SemiCnftagger::readfwit(std::ifstream& in)
{
   std::string line;
   while (std::getline(in,line))
   {
      MyUtil::chomp(line);
      if (MyUtil::IsCommentOut(line.c_str()))
      {
         continue;
      }
      if (line == "End_Fwit")
      {
         break;
      }
      char l,r;
      int i = -1, t = -1;
      std::istringstream is(line);
      is >> l >> i >> r >> t;
      if (i < 0 || t < 0)
      {
         std::cerr << "ERR: " << line << std::endl;
         exit(1);
      }
      this->fwit.push_back(t);
      if (this->fwit[i] != t)
      {
         std::cerr << "ERR: " << line << std::endl;
         exit(1);
      }
   }
}

void SemiCnftagger::readparams(std::ifstream& in)
{
   std::string line;
   while (std::getline(in,line))
   {
      MyUtil::chomp(line);
      if (MyUtil::IsCommentOut(line.c_str()))
      {
         continue;
      }
      if (line == "End_Params")
      {
         break;
      }
      char l,r;
      int i = -1; // index
      float w = 0.; // weight
      std::istringstream is(line);
      is >> l >> i >> r >> w;
      if (i < 0)
      {
         std::cerr << "ERR: " << line << std::endl;
         exit(1);
      }
      this->model[i] = w;
   }
}

void SemiCnftagger::readufeatures(std::ifstream& in)
{
   std::string line;
   while (std::getline(in,line))
   {
      MyUtil::chomp(line);
      if (MyUtil::IsCommentOut(line.c_str()))
      {
         continue;
      }
      if (line == "End_uFeatures")
      {
         break;
      }
      char l, r;
      int i = -1; // index
      std::string f; // feature
      std::istringstream is(line);
      is >> l >> i >> r >> f;
      if (i < 0)
      {
         std::cerr << "ERR: " << line << std::endl;
         exit(1);
      }
      nodeptr n = this->ufeatures->insert(const_cast<char*>(f.c_str()));
      if (n->val != i)
      {
         std::cerr << "ERR: " << line << std::endl;
         exit(1);
      }
   }
}

void SemiCnftagger::readbfeatures(std::ifstream& in)
{
   std::string line;
   while (std::getline(in,line))
   {
      MyUtil::chomp(line);
      if (MyUtil::IsCommentOut(line.c_str()))
      {
         continue;
      }
      if (line == "End_bFeatures")
      {
         break;
      }
      char l, r;
      int i = -1; // index
      std::string f; // feature
      std::istringstream is(line);
      is >> l >> i >> r >> f;
      if (i < 0)
      {
         std::cerr << "ERR: " << line << std::endl;
         exit(1);
      }
      nodeptr n = this->bfeatures->insert(const_cast<char*>(f.c_str()));
      if (n->val != i)
      {
         std::cerr << "ERR: " << line << std::endl;
         exit(1);
      }
   }
}

void SemiCnftagger::readusegments(std::ifstream& in)
{
   std::string line;
   while (std::getline(in,line))
   {
      MyUtil::chomp(line);
      if (MyUtil::IsCommentOut(line.c_str()))
      {
         continue;
      }
      if (line == "End_uSegments")
      {
         break;
      }
      char l, r;
      int i = -1; // index
      std::string s; // segment
      std::istringstream is(line);
      is >> l >> i >> r >> s;
      if (i < 0)
      {
         std::cerr << "ERR: " << line << std::endl;
         exit(1);
      }
      nodeptr n = this->usegments->insert(const_cast<char*>(s.c_str()));
      if (n->val != i)
      {
         std::cerr << "ERR: " << line << std::endl;
         exit(1);
      }
   }
}

void SemiCnftagger::readbsegments(std::ifstream& in)
{
   std::string line;
   while (std::getline(in,line))
   {
      MyUtil::chomp(line);
      if (MyUtil::IsCommentOut(line.c_str()))
      {
         continue;
      }
      if (line == "End_bSegments")
      {
         break;
      }
      char l, r;
      int i = -1; // index
      std::string s; // segment
      std::istringstream is(line);
      is >> l >> i >> r >> s;
      if (i < 0)
      {
         std::cerr << "ERR: " << line << std::endl;
         exit(1);
      }
      nodeptr n = this->bsegments->insert(const_cast<char*>(s.c_str()));
      if (n->val != i)
      {
         std::cerr << "ERR: " << line << std::endl;
         exit(1);
      }
   }
}

void SemiCnftagger::read(const char *model)
{
   if (this->valid)
   {
      std::cerr << "ERR: Model is already initialized" << std::endl;
      exit(1);
   }
   std::ifstream in(model);
   std::string line;
   while (std::getline(in,line))
   {
      MyUtil::chomp(line);
      if (MyUtil::IsCommentOut(line.c_str()))
      {
         continue;
      }
      if (std::strncmp(line.c_str(),"Params",6) == 0)
      {
         this->readparamsize(line);
      }
      else if (std::strncmp(line.c_str(),"lLabels",7) == 0)
      {
         this->readllabelsize(line);
      }
      else if (std::strncmp(line.c_str(),"sLabels",7) == 0)
      {
         this->readslabelsize(line);
      }
      else if (line == "Start_lLabel")
      {
         this->readllabels(in);
      }
      else if (line == "Start_sl2ll")
      {
         this->readmap(in);
      }
      else if (line == "Start_Fwit")
      {
         this->readfwit(in);
      }
      else if (line == "Start_Params")
      {
         this->readparams(in);
      }
      else if (line == "Start_uFeatures")
      {
         this->readufeatures(in);
      }
      else if (line == "Start_bFeatures")
      {
         this->readbfeatures(in);
      }
      else if (line == "Start_uSegments")
      {
         this->readusegments(in);
      }
      else if (line == "Start_bSegments")
      {
         this->readbsegments(in);
      }
   }
   this->umid = this->llabelsize*this->ufeatures->getsize()-1;
   this->bmid = this->umid
      + (2*this->llabelsize+this->llabelsize*this->llabelsize)
      * this->bfeatures->getsize();
   this->usid = this->bmid
      + this->slabelsize*this->usegments->getsize();
   this->bsid = this->usid
      + (2*this->slabelsize+this->slabelsize*this->slabelsize)
      * this->bsegments->getsize();
   this->valid = true;
}

void SemiCnftagger::storefset(Sequence *sq, std::vector<vnode_t>& lattice, AllocMemdiscard *cache)
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
      int ustmpl = 0;
      int bstmpl = 0;
      std::vector<char*> us;
      std::vector<char*> bs;
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
            ustmpl = this->tmpli;
            this->tmpli+=ulen;
         }
         else if (line[0] == 'T')
         {
            blen = this->sexpand(line, sq, i, bs);
            bstmpl = this->tmpli;
            this->tmpli+=blen;
         }
      }
      /// TODO: リファクタすべし
      /// make segments
      std::vector<char*>::iterator usit = us.begin();
      std::vector<char*>::iterator bsit = bs.begin();
      int uslen = 1;
      int bslen = 1;
      int lsize = this->slabelsize;
      while (!(usit == us.end() && bsit == bs.end()))
      {
         vsegment_t s;
         s.sl = -1; // segment label
         s.bl = -1; // local begin label
         s.il = -1; // local inside label
         s.len = uslen;
         s.uid = -1; s.bid = -1;
         s.ut = -1; s.bt = -1;
         s._lcost = 0.;
         s.join = NULL;
         if (uslen < bslen)
         {
            s.len = bslen;
            nodeptr *n = this->bsegments->get(*bsit);
            if (*n != bsnil)
            {
               s.bid = (*n)->val;
               s.bt = bstmpl;
            }
         }
         else if (uslen > bslen)
         {
            nodeptr *n = this->usegments->get(*usit);
            if (*n != usnil)
            {
               s.uid = (*n)->val;
               s.ut = ustmpl;
            }
         }
         else
         {
            nodeptr *n = NULL;
            nodeptr *m = NULL;
            if (usit != us.end())
            {
               n = this->usegments->get(*usit);
            }
            if (bsit != bs.end())
            {
               m = this->bsegments->get(*bsit);
            }
            if (n != NULL && *n != usnil)
            {
               s.uid = (*n)->val;
               s.ut = ustmpl;
            }
            if (m != NULL && *m != bsnil)
            {
               s.bid = (*m)->val;
               s.bt = bstmpl;
            }
         }
         if (usit != us.end())
         {
            this->ac->release(*usit);
            ++usit;
            ++uslen;
            ++ustmpl;
         }
         if (bsit != bs.end())
         {
            this->ac->release(*bsit);
            ++bsit;
            ++bslen;
            ++bstmpl;
         }

         for (int l = 0; l < lsize; ++l)
         {
            vsegment_t *seg = (vsegment_t*)cache->alloc(sizeof(vsegment_t));
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
               lattice[i+seg->len].prev.push_back(seg);
            }
         }
      }
   }
}

float SemiCnftagger::getfw(int id, int tmpl)
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

void SemiCnftagger::initlattice(std::vector<vnode_t>& lattice)
{
   std::vector<vnode_t>::iterator it = lattice.begin();
   int i = 0;
   for (; it != lattice.end(); ++it, ++i)
   {
      std::vector<vsegment_t*>::iterator sit = (*it).next.begin();
      for (; sit != (*it).next.end(); ++sit)
      {
         int len = (*sit)->len;
         int slabel = (*sit)->sl;
         /// unigram segment cost
         int usid = (*sit)->uid;
         int ustmpl = (*sit)->ut;
         if (usid > -1)
         {
            (*sit)->_lcost += this->getfw(this->bmid+1+usid*this->slabelsize+slabel, ustmpl);
         }
         /// token features cost
         for (int j = 0; j < len; ++j)
         {
            feature_t *t = &lattice[i+j].tokenf;
            /// unigram feature in segment
            std::vector<int>::iterator ufit = t->uf.begin();
            std::vector<int>::iterator utit = t->ut.begin();
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

float SemiCnftagger::getbscost(int id, int tmpl, int label)
{
   return this->getfw(id+label, tmpl);
}

float SemiCnftagger::getbfcost(feature_t *f, int bias, int label)
{
   int b = this->umid+1+bias+label;
   float ret = 0.;
   std::vector<int>::iterator bfit = f->bf.begin();
   std::vector<int>::iterator btit = f->bt.begin();
   for (; bfit != f->bf.end(); ++bfit, ++btit)
   {
      int id = b + *bfit * (2*this->llabelsize+this->llabelsize*this->llabelsize);
      int tmpl = *btit;
      ret += this->getfw(id, tmpl);
   }

   return ret;
}

int SemiCnftagger::getbfbias(bool bos, bool eos, int label)
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

int SemiCnftagger::getbsid(bool bos, bool eos, int bid, int label)
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

char* SemiCnftagger::fexpand(std::string& t, Sequence *s, int current)
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

int SemiCnftagger::sexpand(std::string& t, Sequence *s, int current, std::vector<char*>& segments)
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
         for (; *p != ']'; ++p) ;
         *p = '\0';
         tp = p+1;
         suffix += tp;
         /// generate segments
         for (int l = 1; l <= len; ++l)
         {
            std::string str = prefix;
            if (current+l > size)
            {
               continue;
            }
            for (int i = 0; i < l-1; ++i)
            {
               str += s->getToken(current+i,0);
               str += "/";
            }
            str += s->getToken(current+l-1,0);
            str += suffix;
            if (str.size() > 0)
            {
               char *segment = (char*)this->ac->alloc(sizeof(char*)*str.size());
               std::strcpy(segment, str.c_str());
               segments.push_back(segment);
            }
         }
      }
   }
   if (len == 0 && *tp != '\0') /// tonly
   {
      prefix += tp;
      char *segment = (char*)this->ac->alloc(sizeof(char*)*prefix.size());
      std::strcpy(segment,prefix.c_str());
      segments.push_back(segment);
   }
   return len;
}

void SemiCnftagger::viterbi(Sequence *s,
      AllocMemdiscard *cache,
      std::vector<int>& labelids)
{
   int col = s->getRowSize();
   /// build lattice
   std::vector<vnode_t> lattice(col+1);
   /// store feature and segment set
   this->storefset(s, lattice, cache);
   /// init lattice
   this->initlattice(lattice);

   /// viterbi
   std::vector<vnode_t>::iterator it = lattice.begin();
   for (; it != lattice.end(); ++it)
   {
      std::vector<vsegment_t*>::iterator nit = (*it).next.begin();
      for (; nit != (*it).next.end(); ++nit)
      {
         if (it == lattice.begin())
         {
            float c = this->getbfcost(&(*it).tokenf,
                  this->getbfbias((it==lattice.begin()),
                     (it+1==lattice.end()),
                     -1),
                  (*nit)->bl);
            if ((*nit)->bid > -1)
            {
               int id = this->getbsid((it==lattice.begin()),
                     (it+1==lattice.end()),
                     (*nit)->bid,
                     -1);
               c += this->getbscost(id, (*nit)->bt, (*nit)->sl);
            }
            (*nit)->_lcost += c;
         }
         else
         {
            float max = 0;
            vsegment_t *join = NULL;
            std::vector<vsegment_t*>::iterator pit = (*it).prev.begin();
            for (; pit != (*it).prev.end(); ++pit)
            {
               float c = (*pit)->_lcost
                  + this->getbfcost(&(*it).tokenf,
                        this->getbfbias((it==lattice.begin()),
                           (it+1==lattice.end()),
                           (*pit)->il),
                        (*nit)->bl);
               if ((*nit)->bid > -1)
               {
                  int id = this->getbsid((it==lattice.begin()),
                        (it+1==lattice.end()),
                        (*nit)->bid,
                        (*pit)->sl);
                  c += this->getbscost(id, (*nit)->bt, (*nit)->sl);
               }
               if (pit == (*it).prev.begin())
               {
                  max = c; join = *pit;
               }
               if (c > max)
               {
                  max = c; join = *pit;
               }
            }
            (*nit)->join = join;
            (*nit)->_lcost += max;
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
   int ebsid = -1;
   int ebstmpl = -1;
   if (this->tonly)
   {
      nodeptr *n = this->bsegments->get("T");
      ebsid = (*n)->val;
      ebstmpl = this->totmpl;
   }
   --it; // eos
   float max = 0;
   vsegment_t *join = NULL;
   std::vector<vsegment_t*>::iterator pit = (*it).prev.begin();
   for (; pit != (*it).prev.end(); ++pit)
   {
      float c = (*pit)->_lcost + this->getbfcost(&e,
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
      if (pit == (*it).prev.begin())
      {
         max = c; join = *pit;
      }
      if (c > max)
      {
         max = c; join = *pit;
      }
   }

   /// trace back
   vsegment_t *tb = join;
   for (; tb != NULL; tb = tb->join)
   {
      int len = tb->len;
      for (int i = 1; i < len; ++i)
      {
         labelids.push_back(tb->il);
      }
      labelids.push_back(tb->bl);
   }
}

void SemiCnftagger::output(Sequence *s, std::vector<int>& labels)
{
   int row = s->getRowSize();
   std::vector<int>::reverse_iterator rit = labels.rbegin();
   for (int i = 0; i < row; ++i, ++rit)
   {
      for (int j = 0; j < (int)this->sqcolsize; ++j)
      {
         std::cout << s->getToken(i,j) << ' ';
      }
      std::cout << this->label2surf[*rit] << std::endl;
   }
   std::cout << std::endl;
}

void SemiCnftagger::tagging(const char *corpus)
{
   if (corpus == NULL)
   {
      std::cerr << "Corpus is NULL" << std::endl;
      return;
   }
   if (!this->valid)
   {
      std::cerr << "It's not initialized" << std::endl;
      return;
   }
   AllocMemdiscard cache(this->cachesize);
   Sequence sq;
   sq.setColSize(this->sqcolsize);
   sq.init();

   std::ifstream in(corpus);
   std::string line;
   while (!in.eof())
   {
      MyUtil::sqread(in, &sq);
      if (sq.getRowSize() == 0)
      {
         continue;
      }
      std::vector<int> lids;
      this->viterbi(&sq, &cache, lids);
      if (sq.getRowSize() != lids.size())
      {
         std::cerr << "ERR: sqsize != labels" << std::endl
            << sq.getRowSize() << ' ' << lids.size() << std::endl;
         exit(1);
      }
      this->output(&sq, lids);
      sq.clear();
      cache.reset();
   }
}
