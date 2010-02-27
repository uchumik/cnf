# include <dic.h>
# include <tmpl.h>
# include <sequence.h>
# include <myutil.h>
# include <zlib.h>
# include <fcntl.h>
# include <cstdio>
# include <cstdlib>
# include <list>
# define BUFSIZE 4096
# define GZ_MODE "rb6f"

int recalldump (nodeptr nil, nodeptr p, int bound)
{
   if (p != nil)
   {
      recalldump(nil, p->left, bound);
      recalldump(nil, p->right, bound);

      if (p->val > bound)
      {
        fprintf (stdout, "%s\t%d\n",p->key, p->val);
      }
   }
   return 0;
}

int main (int argc, char **argv)
{
   if (argc < 5)
   {
      fprintf (stderr, "template inputfile.gz colsize bound\n");
      exit (1);
   }
   FILE *fp = NULL;
   if ((fp = fopen(*(argv+1),"r")) == NULL)
   {
      fprintf (stderr, "Couldn't open %s\n", *(argv+1));
      exit (1);
   }
   PoolAlloc ac(256, 1000000);
   std::list<tmpl*> tmpls;
   char buf[BUFSIZE];
   while (fgets(buf, BUFSIZE, fp) != NULL)
   {
      MyUtil::chomp(buf);
      if (MyUtil::IsCommentOut(buf))
      {
         continue;
      }
      tmpl *t = new tmpl(buf, &ac);
      tmpls.push_back(t);
   }

   int fd = open(*(argv+2), O_RDONLY);
   if (fd < 0)
   {
      fprintf (stderr, "Couldn't open %s\n",*(argv+2));
      exit (1);
   }
   gzFile input = gzdopen(fd, GZ_MODE);
   if (input == NULL)
   {
      fprintf (stderr, "failed to gzdopen\n");
      exit (1);
   }

   int colsize = 0;
   sscanf (*(argv+3),"%d",&colsize);
   int bound = 0;
   sscanf (*(argv+4),"%d",&bound);

   Sequence sq;
   sq.setColSize(colsize);
   sq.init();

   Dic features(&ac, CountUp);

   while (gzgets(input, buf, BUFSIZE) != NULL)
   {
      MyUtil::chomp(buf);
      if (MyUtil::IsEOS(buf))
      {
         int size = (int)sq.getRowSize();
         std::list<tmpl*>::iterator it = tmpls.begin();
         for (; it != tmpls.end(); it++)
         {
            for (int i = 0; i < size; i++)
            {
               char *feature = (*it)->expand(&sq,i);
               //fprintf(stdout, "%s\n",feature);
               features.insert(feature);
               ac.release(feature);
            }
         }
         sq.clear();
         continue;
      }
      sq.push(buf);
   }

   nodeptr nil = features.getnil();
   for (int i = HASHSIZE-1; i >= 0; i--)
   {
      nodeptr *p = features.table+i;
      if (*p != nil)
      {
         recalldump(nil, (*p)->left, bound);
         recalldump(nil, (*p)->right, bound);
      }
   }

   if (gzclose(input) != Z_OK)
   {
      fprintf (stderr,"gzclose failed\n");
      exit (1);
   }
   return 0;
}
