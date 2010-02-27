/**
 * written by kuchiumi
 * implementated Pool Allocation Strategy
 * Copyright (C) 2010- Kei Uchiumi
 */


# ifndef __ALLOC_PL_H__
# define __ALLOC_PL_H__

# include <cstdio>

class PoolAlloc
{
   public:
      PoolAlloc(size_t blocksize, size_t poolsize);
      ~PoolAlloc();

      void* alloc(size_t size);
      int release(void *p);
   private:
      PoolAlloc();
      PoolAlloc(const PoolAlloc&);
      PoolAlloc& operator=(const PoolAlloc&);

      int allocPool();
      void addToFreelist(void *node);

      size_t nbyteAllocated;
      size_t blocksize;
      size_t poolsize;
      size_t range;

      void *freelist;
      char *head;
};

# endif /* __ALLOC_PL_H__ */
