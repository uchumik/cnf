# ifndef CNF_LEARNER
# define CNF_LEARNER

# include <cnflearn.h>
# include <semicnflearn.h>

template<class T>
class Learner
{
   public:
      Learner(const char *tmpl, const char *corpus, unsigned int poolsize);
      ~Learner();
      void learn(unsigned int iter, unsigned int reg);
      void save(const char *save);
      bool init();
      void setcache(unsigned int cachesize);
      void setpenalty(float w, float u, float t);
      void setpenalty(float bs, float us, float bf, float uf, float t);
      void setlabelcol(unsigned int labelcol);
      void setsqcol(unsigned int sqcolsize);
      void setbound(unsigned int bound);
      void setfbound(unsigned int fbound);
      void setsbound(unsigned int sbound);
      void setlambda(float lambda);
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
