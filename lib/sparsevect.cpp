/**
 * Copyright (C) 2010- Kei Uchiumi
 */
# include <sparsevect.h>
# include <cstdio>
# include <vector>

struct fact nilf = {NULL, NULL, -1, 0.};

SparseVector::SparseVector(AllocMemdiscard *ac)
{
	this->ac = ac;
	factptr p;
	int size = (int)HASHSIZE * sizeof(*p);
	this->table = static_cast<factptr*>(this->ac->alloc(size));
	this->reset();
}

SparseVector::~SparseVector()
{
}

SpvState SparseVector::reset()
{
	for (int i = 0; i < HASHSIZE; i++)
	{
		*(this->table+i) = &nilf;
	}
	return SpvOk;
}

SpvState SparseVector::add(int k, double v)
{
	int dist = 0;
	factptr *p = (this->table+SparseVector::hash(k));
	while ((dist=(*p)->key-k) != 0 && *p != &nilf)
	{
		if (dist > 0)
		{
			p = &((*p)->left);
		}
		else
		{
			p = &((*p)->right);
		}
	}
	if (*p != &nilf)
	{
		(*p)->val += v;
		return SpvOk;
	}
	factptr n;
	int size = sizeof(*n);
	n = static_cast<factptr>(this->ac->alloc(size));
	n->key = k;
	n->val = v;
	n->left = &nilf;
	n->right = *p;
	*p = n;
	this->keys.push_back(k);
	return SpvOk;
}

double SparseVector::get(int k)
{
	int dist = 0;
	factptr *p = (this->table+SparseVector::hash(k));
	while((dist=(*p)->key-k) != 0 && *p != &nilf)
	{
		if (dist > 0)
		{
			p = &((*p)->left);
		}
		else
		{
			p = &((*p)->right);
		}
	}

	return (*p)->val;
}
