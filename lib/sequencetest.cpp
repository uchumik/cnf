# include <sequence.h>
# include <myutil.h>
# include <zlib.h>
# include <fcntl.h>
# include <cstdio>
# include <cstdlib>
# define BUFSIZE 4096
# define GZ_MODE "rb6f"

int main (int argc, char **argv)
{
   if (argc < 3)
   {
      fprintf (stderr, "input file.gz colsize\n");
      exit (1);
   }

   int fd = open(*(argv+1), O_RDONLY);
   if (fd < 0)
   {
      fprintf (stderr, "Couldn't open %s\n",*(argv+1));
      exit (1);
   }
   gzFile input = gzdopen(fd, GZ_MODE);
   if (input == NULL)
   {
      fprintf (stderr, "failed to gzdopen\n");
      exit (1);
   }

   int colsize = 0;
   sscanf (*(argv+2),"%d",&colsize);

   Sequence sq;
   sq.setColSize(colsize);
   sq.init();

   char buf[BUFSIZE];
   while(gzgets(input, buf, BUFSIZE) != NULL)
   {
      MyUtil::chomp(buf);
      if (MyUtil::IsEOS(buf))
      {
         sq.dump();
         sq.clear();
         continue;
      }
      sq.push(buf);
   }

   if (gzclose(input) != Z_OK)
   {
      fprintf (stderr,"gzclose failed\n");
      exit (1);
   }
   return 0;
}
