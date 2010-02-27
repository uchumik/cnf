/**
 * Copyright (C) 2010- Kei Uchiumi
 */
# include <allocmd.h>
# include <cstdlib>
# include <memory>

AllocMemdiscard::AllocMemdiscard(size_t heapsize)
{
   try
   {
      this->memHeap = new char[heapsize];
   }
   catch (std::bad_alloc e)
   {
      fprintf (stderr, "ERR: Failed to allocate %zd\n", heapsize);
   }
   this->nbyteHeapSize = heapsize;
   this->reset();
}

AllocMemdiscard::~AllocMemdiscard()
{
   delete[] (char*)this->memHeap;
}

void* AllocMemdiscard::alloc(size_t objectsize)
{
   if (this->nbyteAllocated + objectsize >= this->nbyteHeapSize)
   {
      fprintf(stderr,"Memsize over!\n");
      exit(1);
   }
   void *allocatedCell = this->memHeap + this->nbyteAllocated;
   this->nbyteAllocated += objectsize;

   return allocatedCell;
}

void AllocMemdiscard::reset()
{
   this->nbyteAllocated = 0;
}
