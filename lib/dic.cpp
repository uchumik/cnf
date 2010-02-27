/**
 * Copyright (C) 2010- Kei Uchiumi
 */
# include <dic.h>
# include <cstring>

struct dicnode nil = {NULL,NULL,"\0",0};

Dic::Dic (PoolAlloc *ac, DicMode mode)
{
   this->ac = ac;
   this->mode = mode;
   nodeptr p;
   int size = (int)(HASHSIZE) * (int)sizeof(*p);
   this->wordnum = 0;
   this->table = (nodeptr*)(this->ac->alloc(size));
   this->init();
}

Dic::Dic ()
{
}

Dic::~Dic ()
{
   for (int i = HASHSIZE-1; i >= 0; i--)
   {
      nodeptr *p = (this->table+i);
      if (*p == &nil)
      {
         continue;
      }
      this->recallfree(*p);
   }
   this->ac->release(this->table);
}

nodeptr Dic::getnil ()
{
   return &nil;
}

int Dic::recallfree (nodeptr p)
{
   if (p != &nil)
   {
      this->recallfree(p->left);
      this->recallfree(p->right);
      this->ac->release(p->key);
      this->ac->release(p);
   }
   return 0;
}

int Dic::init ()
{
   for (int i = 0; i < HASHSIZE; i++)
   {
      *(this->table+i) = &nil;
   }
   return 0;
}

int Dic::getsize ()
{
   return this->wordnum;
}

nodeptr Dic::insert (char *word)
{
   int dist = 0;
   nodeptr *p = (this->table+Dic::hash(word));
   while ( (dist = std::strcmp( (*p)->key, word ) ) != 0 && *p != &nil )
   {
      if (dist < 0)
      {
         p = &((*p)->left);
      }
      else
      {
         p = &((*p)->right);
      }
   }

   if (*p != &nil) // stored
   {
      if (this->mode == CountUp)
      {
         (*p)->val++;
      }
      return NULL;
   }

   nodeptr n;
   int size = sizeof(*n);
   int len = std::strlen(word) + 1;
   n = (nodeptr)this->ac->alloc(size);
   n->key = (char*)this->ac->alloc(len);
   std::strncpy(n->key,word,len);
   *(n->key+len) = '\0';
   if (this->mode == CountUp)
   {
      n->val = 1;
      this->wordnum++;
   }
   else if (this->mode == Index)
   {
      n->val = this->wordnum++;
   }
   n->left = &nil;
   n->right = *p;
   *p = n;

   return *p;
}

nodeptr* Dic::get (char *word)
{
   int dist = 0;
   nodeptr *p = (this->table+Dic::hash(word));
   while ( (dist = std::strcmp( (*p)->key, word ) ) != 0 && *p != &nil )
   {
      if (dist < 0)
      {
         p = &((*p)->left);
      }
      else
      {
         p = &((*p)->right);
      }
   }
   return p;
}

float Dic::getfillingrate ()
{
   int count = 0;
   for (int i = 0; i < HASHSIZE; i++)
   {
      nodeptr *p = (this->table+i);
      if (*p == &nil)
      {
         continue;
      }
      count++;
   }

   return (float)count/HASHSIZE;
}
