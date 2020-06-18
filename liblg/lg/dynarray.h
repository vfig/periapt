#ifndef LG_DYNARRAY_H
#define LG_DYNARRAY_H

#if _MSC_VER > 1000
#pragma once
#endif

#include <stdexcept>
#include <new>

#ifdef __GNUC__
#pragma interface "lg/dynarray.h"
#endif

class cDynArrayBase
{
protected:
	void * m_data;
	unsigned long m_size;

	static void _resize(void **p, unsigned int sz, unsigned int c) throw(/*std::bad_alloc*/);

	~cDynArrayBase();
	cDynArrayBase()
		: m_data(NULL), m_size(0) { }

};

template <typename _T>
class cDynArray : protected cDynArrayBase
{
private:
	typedef _T		type;
	typedef _T*		ptr;
	typedef _T&		ref;
	typedef const _T*		c_ptr;
	typedef const _T&		c_ref;

public:
	cDynArray()
		: cDynArrayBase() { }
	cDynArray(unsigned int _i);

	cDynArray(const cDynArray<_T>& _cpy);
	cDynArray<_T>& operator=(const cDynArray<_T>& _cpy);

	unsigned long size() const
		{ return m_size; }
	ref operator[](int _n) throw(/*std::out_of_range*/);
	c_ref operator[](int _n) const throw(/*std::out_of_range*/);
};

#include <lg/dynarray.hpp>

#endif
