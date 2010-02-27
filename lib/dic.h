/**
 * Copyright (C) 2010- Kei Uchiumi
 */
# ifndef __DIC_H__
# define __DIC_H__

# include <allocpl.h>
extern "C"
{
# include <limits.h>
}

//# define HASHSIZE 1500007
# define HASHSIZE 2597


typedef enum
{
   CountUp,
   Index
} DicMode;

typedef struct dicnode {
   struct dicnode *left, *right;
   char *key;
   int val;
} *nodeptr;

class Dic
{
   public:
      Dic(PoolAlloc *ac, DicMode mode);
      ~Dic();
      /**
       * @param[in] keyword
       */
      nodeptr insert(char *word);
      /**
       * @param[in] keyword
       * @param[out] node
       */
      nodeptr* get(char *word);
      /**
       * @param[out] stored word num
       */
      int getsize();
      /**
       * @param[out] nil nodeptr
       */
      nodeptr getnil();
      /**
       * @param[out] filling rate of table
       */
      float getfillingrate();
      nodeptr *table;
   private:
      Dic();
      Dic(const Dic&);
      Dic& operator=(const Dic&);

      int init();
      static inline int Dic::hash(char *s)
      {
         unsigned int v;
         for (v = 0; *s != '\0'; s++)
         {
            v = ((v << CHAR_BIT) + *s) % HASHSIZE;
         }
         return (int)v;
      }

      int recallfree(nodeptr p);
      PoolAlloc *ac;
      int wordnum;
      DicMode mode;
};
# endif /* __DIC_H__ */
