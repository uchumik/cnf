/**
 * Copyright (C) 2010- Kei Uchiumi
 */
# include <allocpl.h>
# include <cstdlib>
# include <memory>

PoolAlloc::PoolAlloc (size_t blocksize, size_t poolsize)
{
   this->blocksize = blocksize;
   this->poolsize = poolsize;
   this->freelist = NULL;

   if ( this->allocPool() )
   {
      fprintf (stderr, "ERR: Couldn't allocate\n");
      exit(1);
   }
}

PoolAlloc::~PoolAlloc ()
{
   for (; *((void**)this->freelist) != NULL;
         this->freelist = *((void**)this->freelist) )
   {
   }
   delete[] (char*)this->freelist;
}

void PoolAlloc::addToFreelist (void *node)
{
   *((void**)node) = this->freelist;
   this->freelist = node;
}

int PoolAlloc::allocPool ()
{
   char *node = NULL;
   try
   {
      this->range = this->blocksize * this->poolsize;
      node = new char[this->range];
      this->head = node;
   }
   catch (std::bad_alloc e)
   {
      fprintf (stderr, "ERR: Failed to allocate %zd\n", this->range);
      exit (1);
   }

   for (unsigned int i = 0; i < this->poolsize; i++)
   {
      this->addToFreelist (node + (i * this->blocksize) );
   }

   return 0;
}

void* PoolAlloc::alloc (size_t bytesToAllocate)
{
   if (bytesToAllocate > this->blocksize)
   {
      return ::operator new (bytesToAllocate);
   }

   if (this->freelist == 0)
   {
      fprintf (stderr, "ERR: freelist points to NULL\n");
      exit (1);
   }

   void *node = this->freelist;
   this->freelist = *((void**)node);

   return node;
}

int PoolAlloc::release (void *p)
{
   if (p >= this->head && p < this->head+this->range)
   {
      this->addToFreelist (p);
   }
   else
   {
      ::operator delete(p);
   }

   return 0;
}
