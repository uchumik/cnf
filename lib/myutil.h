/**
 * written by kuchiumi
 * util class for folos crf
 */

# ifndef __MYUTIL_H__
# define __MYUTIL_H__

# include <cstdio>
# include <cstring>
# include <sequence.h>
# include <string>
# include <iostream>
# include <fstream>

class MyUtil
{
   public:
      MyUtil();
      ~MyUtil();

      static inline bool IsEOS (const char *str)
      {
         if (std::strcmp(str,"") == 0)
         {
            return true;
         }
         return false;
      }

      static inline bool IsEOS (std::string& s)
      {
         if (s == "")
         {
            return true;
         }
         return false;
      }

      static inline void chomp (char *str)
      {
         int len = std::strlen(str);
         if (*(str+len-1) == '\n')
         {
            *(str+len-1) = '\0';
            --len;
         }
         if (*(str+len-1) == '\r')
         {
            *(str+len-1) = '\0';
         }
      }

      static inline void chomp (std::string& s)
      {
         int len = s.size();
         if (s[len-1] == '\n')
         {
            s[len-1] = '\0';
            --len;
         }
         if (s[len-1] == '\r')
         {
            s[len-1] = '\0';
         }
      }

      static inline unsigned int getByteUtf8 (const char *p)
      {
         const unsigned char c = *p;
         if (c <= 0x7F)
         {
            return 1; // ascii
         }
         else if (c <= 0xDF)
         {
            return 2;
         }
         else if (c <= 0xF7)
         {
            return 3;
         }
         else if (c < 0xFB)
         {
            return 4;
         }
         // undefined over than 4byte
         // 5byte 以降は登録される計画も無い
         return (unsigned int)-1;
      }

      static inline void push (int c, char **s)
      {
         *(*s)++ = c;
      }

      static inline void itoalter (int n, char **s)
      {
         if (n != 0)
         {
            itoalter (n / 10, s);
            push ('0' + n % 10, s);
         }
      }

      static inline void itoa (int n, char *s)
      {
         if (n < 0)
         {
            push ('-', &s);
            n = -n;
         }
         itoalter (n, &s);
         push ('\0', &s);
      }

      static inline bool IsCommentOut (const char *str)
      {
         for (; *str != '\0' && (*str == ' ' || *str == '\t'); str++) ;
         if (std::strlen(str) == 0)
         {
            return true;
         }
         else if (*str == '#')
         {
            return true;
         }
         return false;
      }
      /**
       * read sequence function
       * @param fp   FILE pointer
       * @param sq   Sequence
       * @param bufsize buffer size
       */
      static void sqread(FILE *fp, Sequence *s, unsigned int bufsize)
      {
         char buf[bufsize];
         while (fgets(buf,bufsize,fp) != NULL)
         {
            MyUtil::chomp(buf);
            if (MyUtil::IsEOS(buf))
            {
               break;
            }
            s->push(buf);
         }
      }
      /**
       * read sequence function
       * @param in   ifstream
       * @param sq   Sequence
       */
      static void sqread(std::ifstream& in, Sequence *s)
      {
         std::string line;
         while (std::getline(in,line))
         {
            MyUtil::chomp(line);
            if (MyUtil::IsEOS(line))
            {
               break;
            }
            s->push(line.c_str());
         }
      }
   private:

};
# endif /* __MYUTIL_H__ */
