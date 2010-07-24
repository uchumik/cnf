# ifndef CNF_LEARNER
# define CNF_LEARNER

# include <cnflearn.h>
# include <semicnflearn.h>

template<class T>
class Learner
{
   public:
      /** Construct Learner
       * @param tmpl template
       * @param corpus training-corpus
       * @param poolsize poolsize for allocator
       */
      Learner(const char *tmpl, const char *corpus, unsigned int poolsize);
      ~Learner();
      /** Learn Model Parameters
       * @param iter iteration
       * @param regularize 0:L1-regularization 1:L2-regularization
       */
      void learn(unsigned int iter, unsigned int reg);
      /** Save Model Parameters
       * @param save modelfile
       */
      void save(const char *save);
      /** Initialize */
      bool init();
      /** Set cachesize
       * @param cache cachesize
       */
      void setcache(unsigned int cachesize);
      /** Set penalty parameters
       * @param w penalty parameter for observed-feature
       * @param u penalty parameter for transition-feauture
       * @param t penalty parameter for gate-function
       */
      void setpenalty(float w, float u, float t);
      /** Set penalty parameters
       * @param bs penalty parameter for transition-feature of segment
       * @param us penalty parameter for observed-feature of segment
       * @param bf penalty parameter for transition-feauture of token
       * @param uf penalty parameter for observed-feauture of token
       * @param t penalty parameter for gate-function
       */
      void setpenalty(float bs, float us, float bf, float uf, float t);
      /** Set label-col in sequence
       * @param labelcol labelcol
       */
      void setlabelcol(unsigned int labelcol);
      /** Set colsize of sequence
       * @param colsize colsize of sequence
       */
      void setsqcol(unsigned int sqcolsize);
      /** Set threshold of cut-off for feature
       * @param bound NUM
       */
      void setbound(unsigned int bound);
      /** Set threshold of cut-off for token feature
       * @param bound NUM
       */
      void setfbound(unsigned int fbound);
      /** Set threshold of cut-off for segment feature
       * @param bound NUM
       */
      void setsbound(unsigned int sbound);
      /** Set Lambda parameter for detecting learning-rate eta
       * @param lambda lambda
       */
      void setlambda(float lambda);
      /** Set Alpha parameter for detecting learning-rate eta
       * @param alpha alpha
       */
      void setalpha(float alpha);
   private:
      Learner();
      Learner(const Learner&);
      Learner& operator=(const Learner&);

      T *impl;
};

   template<class T>
Learner<T>::Learner(const char *tmpl, const char *corpus, unsigned int poolsize)
{
   this->impl = new T(tmpl,corpus,poolsize);
}

   template<class T>
Learner<T>::~Learner()
{
   delete this->impl;
}

   template<class T>
void Learner<T>::learn(unsigned int iter, unsigned int reg)
{
   this->impl->learn(iter,reg);
}

   template<class T>
void Learner<T>::save(const char *save)
{
   this->impl->save(save);
}

   template<class T>
bool Learner<T>::init()
{
   return this->impl->init();
}

   template<class T>
void Learner<T>::setcache(unsigned int cachesize)
{
   this->impl->setcache(cachesize);
}

   template<class T>
void Learner<T>::setlabelcol(unsigned int labelcol)
{
   this->impl->setlabelcol(labelcol);
}

   template<class T>
void Learner<T>::setsqcol(unsigned int sqcolsize)
{
   this->impl->setsqcol(sqcolsize);
}

   template<class T>
void Learner<T>::setlambda(float lambda)
{
   this->impl->setlambda(lambda);
}

   template<>
void Learner<Cnf::Cnflearn>::setpenalty(float w, float u, float t)
{
   this->impl->setpenalty(w,u,t);
}

   template<>
void Learner<SemiCnf::SemiCnflearn>::setpenalty(float bs, float us, float bf, float uf, float t)
{
   this->impl->setpenalty(bs,us,bf,uf,t);
}

   template<>
void Learner<Cnf::Cnflearn>::setbound(unsigned int bound)
{
   this->impl->setbound(bound);
}

   template<>
void Learner<SemiCnf::SemiCnflearn>::setfbound(unsigned int fbound)
{
   this->impl->setfbound(fbound);
}

   template<>
void Learner<SemiCnf::SemiCnflearn>::setsbound(unsigned int sbound)
{
   this->impl->setsbound(sbound);
}

   template<>
void Learner<SemiCnf::SemiCnflearn>::setalpha(float alpha)
{
   this->impl->setalpha(alpha);
}
# endif /* CNF_LEARNER */
