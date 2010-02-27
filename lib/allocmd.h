/**
 * written by kei.uchiumi
 * implementated Memory Discard Strategy
 * Copyright (C) 2010- Kei Uchiumi
 */

# ifndef __ALLOC_MD_H__
# define __ALLOC_MD_H__

# include <cstdio>

class AllocMemdiscard
{
   public:
      AllocMemdiscard(size_t);
      ~AllocMemdiscard();
      void* alloc(size_t);
      void reset();
   private:
      AllocMemdiscard();
      AllocMemdiscard(const AllocMemdiscard&);
      AllocMemdiscard& operator=(const AllocMemdiscard&);

      size_t nbyteAllocated;
      size_t nbyteHeapSize;
      char *memHeap;
};
# endif /* __ALLOC_MD_H__ */
