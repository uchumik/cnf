# ifndef CNF_TAGGER
# define CNF_TAGGER

# include <cnftagger.h>
# include <semicnftagger.h>

template<class T>
class Tagger
{
   public:
      /** Construct Tagger
       * @param tmpl template
       * @param poolsize poolsize for allocator
       */
      Tagger(const char *tmpl, unsigned int poolsize);
      ~Tagger();
      /** Read Modelfile
       * @param model modelfile
       */
      void read(const char *model);
      /** Tagging to corpus
       * @param corpus test-corpus
       */
      void tagging(const char *corpus);
      /** Predict Tags and set to labels
       * @param s target sequence
       * @param cache cache
       * @param labels vector to store predicted labels
       */
      void viterbi(Sequence *s,
            AllocMemdiscard *cache,
            std::vector<int>& labels);
      /** Output labeled-sequence
       * @param s sequence
       * @param labels vector stored labels
       */
      void output(Sequence *s, std::vector<int>& labels);
      /** Set cachesize
       * @param cache cachesize
       */
      void setcache(unsigned int cachesize);
      /** Set colsize of sequence
       * @param colsize colsize of sequence
       */
      void setsqcol(unsigned int sqcolsize);
      /** Set arraysize of sequence
       * @param arraysize number of tokens in sequence
       */
      void setsqarraysize(unsigned int sqarraysize);
      /** Set alloc-size of sequence
       * @param allocsize allocsize
       */
      void setsqallocsize(unsigned int sqallocsize);
      /** Clear model parameter */
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

   template<class T>
void Tagger<T>::setsqarraysize(unsigned int sqarraysize)
{
   this->impl->setsqarraysize(sqarraysize);
}

   template<class T>
void Tagger<T>::setsqallocsize(unsigned int sqallocsize)
{
   this->impl->setsqallocsize(sqallocsize);
}

   template<>
void Tagger<Cnf::Cnftagger>::clear()
{
   this->impl->clear();
}
# endif /* CNF_TAGGER */
