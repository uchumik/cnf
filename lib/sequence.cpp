/**
 * Copyright (C) Kei Uchiumi
 */
# include <myutil.h>
# include <sequence.h>
# include <cstdio>
# include <cstring>

const char *sp = " ";

Sequence::Sequence ()
{
   this->acsize = ALLOCSIZE;
   this->arraysize = (unsigned int)(this->acsize/BLOCKSIZE);
   this->ac = NULL;
   this->delim = sp;
   this->colsize = 1;
   this->ngram = 1;
}

Sequence::~Sequence ()
{
   this->ac->reset();
   delete this->ac;
}

int Sequence::dump ()
{
   for (unsigned int i = 1; i <= this->point/this->colsize; i++)
   {
      for (unsigned int j = (i-1)*this->colsize; j < i*this->colsize; j++)
      {
         fprintf (stdout, "%s\t",*(this->tokens+j));
      }
      fprintf (stdout, "\n");
   }
   fprintf (stdout, "\n");
   return 0;
}

int Sequence::init ()
{
   if (this->ac != NULL)
   {
      delete this->ac;
   }

   this->ac = new AllocMemdiscard(this->acsize);
   this->tokens = (char**)(this->ac->alloc(sizeof(char*)*this->arraysize));
   this->point = 0;

   return 0;
}

int Sequence::setAllocSize (unsigned int size)
{
   this->acsize = size;
   //this->arraysize = (unsigned int)this->acsize/BLOCKSIZE;
   return 0;
}

int Sequence::setArraySize (unsigned int arraysize)
{
   this->arraysize = arraysize;
   return 0;
}

int Sequence::setColSize (unsigned int size)
{
   this->colsize = size;
   return 0;
}

int Sequence::push (const char *str)
{
   const char *head = str;
   const char *p = str;
   unsigned int shift = 0;
   unsigned int col = 0;
   while (shift = MyUtil::getByteUtf8(str))
   {
      if (std::strncmp(str, this->delim, std::strlen(this->delim)) == 0)
      {
         if (this->point == this->arraysize)
         {
            fprintf (stderr, "arraysize over[%u]\n",this->arraysize);
            exit(1);
         }
         int len = str - p;
         char *token = (char*)this->ac->alloc(len+1);
         std::strncpy(token,p,len);
         *(token+len) = '\0';
         *(this->tokens+this->point++) = token;
         p = str+1;
         ++col;
      }
      str += shift;
      if (std::strlen(str) == 0)
      {
         break;
      }
   }
   if (std::strlen(p) > 0)
   {
      if (this->point == this->arraysize)
      {
         fprintf (stderr, "arraysize over[%u]\n",this->arraysize);
         exit(1);
      }
      int len = str - p;
      char *token = (char*)this->ac->alloc(len+1);
      std::strncpy(token,p,len);
      *(token+len) = '\0';
      *(this->tokens+this->point++) = token;
      ++col;
   }
   if (col != this->colsize)
   {
      fprintf (stderr, "pushed tokens num != colsize[%u]\n",this->colsize);
      fprintf (stderr, "%s\n",head);
      exit (1);
   }
   return col;
}

unsigned int Sequence::getRowSize ()
{
   return this->point/this->colsize;
}

int Sequence::clear ()
{
   this->ac->reset();
   this->tokens = (char**)this->ac->alloc(sizeof(char*)*this->arraysize);
   this->point = 0;
   return 0;
}

char* Sequence::getToken (int row, int col)
{
   if (col >= (int)this->colsize)
   {
      return NULL;
   }
   // BOS
   if (row < 0)
   {
      const char *head = "_B";
      char tail[64] = "\0";
      MyUtil::itoa(row,tail);
      int len = std::strlen(head) + std::strlen(tail);
      char *bos = (char*)this->ac->alloc(len+1);
      std::strcpy(bos,head);
      std::strcat(bos,tail);
      *(bos+len) = '\0';
      return bos;
   }
   // EOS
   else if (row >= (int)(this->point/this->colsize))
   {
      const char *head = "_E+";
      char tail[64] = "\0";
      MyUtil::itoa(row+(1-(int)this->point/this->colsize),tail);
      int len = std::strlen(head) + std::strlen(tail);
      char *eos = (char*)this->ac->alloc(len+1);
      std::strcpy(eos,head);
      std::strcat(eos,tail);
      *(eos+len) = '\0';
      return eos;
   }
   // default
   else if (row < (int)(this->point/this->colsize) && col < (int)this->colsize)
   {
      return *(this->tokens+row*this->colsize+col);
   }
   return NULL;
}
