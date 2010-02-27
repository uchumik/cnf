/**
 * written by kei.uchiumi
 * sequence class
 * Copyright (C) 2010- Kei Uchiumi
 */

# ifndef __SEQUENCE_H__
# define __SEQUENCE_H__

# include <cstdio>
# include <allocmd.h>

# define ALLOCSIZE (4096*1000) // 512KB
# define BLOCKSIZE 4096

class Sequence
{
   public:
      Sequence();
      ~Sequence();
      /**
       *
       */
      int dump();
      /**
       * @param[in] str
       */
      int push(const char *str);
      /**
       *
       */
      int clear();
      /**
       * @param[out] rowsize
       */
      unsigned int getRowSize();
      /**
       * @param[in] row
       * @param[in] col
       * @param[out] char*
       */
      char* getToken(int row, int col);
      /**
       * @param[in] delimit
       */
      int setDelimit(const char *delimit);
      /**
       *
       */
      int init();
      /**
       * @param[in] allocsize
       */
      int setAllocSize(unsigned int allocsize);
      /**
       * @param[in] colsize
       */
      int setColSize(unsigned int colsize);
      /**
       * @param[in] char
       */
   private:
      Sequence(const Sequence&);
      Sequence& operator=(const Sequence&);

      unsigned int acsize;
      char **tokens;
      AllocMemdiscard *ac;
      const char *delim;

      unsigned int colsize;
      unsigned int arraysize;
      unsigned int point;

      unsigned int ngram;
};

# endif /* __SEQUENCE_H__ */
