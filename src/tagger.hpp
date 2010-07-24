# ifndef CNF_TAGGER
# define CNF_TAGGER

# include <cnftagger.h>
# include <semicnftagger.h>

template<class T>
class Tagger
{
   public:
      Tagger(const char *tmpl, unsigned int poolsize);
      ~Tagger();
      void read(const char *model);
      void tagging(const char *corpus);
      void viterbi(Sequence *s,
            AllocMemdiscard *cache,
            std::vector<int>& labels);
      void output(Sequence *s, std::vector<int>& labels);
      void setcache(unsigned int cachesize);
      void setsqcol(unsigned int sqcolsize);
      void clear();
   private:
      Tagger();
      Tagger(const Tagger&);
      Tagger& operator=(const Tagger&);

      T *impl;
};

   template<class T>
Tagger<T>::Tagger(const char *tmpl, unsigned int poolsize)
{
   this->impl = new T(tmpl,poolsize);
}

   template<class T>
Tagger<T>::~Tagger()
{
   delete this->impl;
}

   template<class T>
void Tagger<T>::read(const char *model)
{
   this->impl->read(model);
}

   template<class T>
void Tagger<T>::tagging(const char *corpus)
{
   this->impl->tagging(corpus);
}

   template<class T>
void Tagger<T>::viterbi(Sequence *s,
      AllocMemdiscard *cache,
      std::vector<int>& labels)
{
   this->impl->viterbi(s,cache,labels);
}

   template<class T>
void Tagger<T>::output(Sequence *s,
      std::vector<int>& labels)
{
   this->impl->output(s,labels);
}

   template <class T>
void Tagger<T>::setcache(unsigned int cachesize)
{
   this->impl->setcache(cachesize);
}

   template <class T>
void Tagger<T>::setsqcol(unsigned int sqcolsize)
{
   this->impl->setsqcol(sqcolsize);
}

   template<>
void Tagger<Cnf::Cnftagger>::clear()
{
   this->impl->clear();
}
# endif /* CNF_TAGGER */
