/**********************
 * LGS Types
 */

#ifndef _LG_TYPES_H
#define _LG_TYPES_H

#if _MSC_VER > 1000
#pragma once
#endif

#include <lg/config.h>
#include <lg/objstd.h>

#ifdef __GNUC__
#pragma interface "lg/types.h"
#endif


struct true_bool
{
	unsigned int f;

	true_bool() : f(0) { }
	true_bool(int v) { f = v; }
	
	bool operator== (int v) const
	{
		return (f) ? (v != 0) : (v == 0);
	}
	bool operator!= (int v) const
	{
		return (f) ? (v == 0) : (v != 0);
	}
	bool operator! () const
	{
		return f == 0;
	}

	true_bool& operator= (int v)
	{
		f = v;
		return *this;
	}
	
	operator int () const
	{
		return (f) ? 1 : 0;
	}
	operator bool () const
	{
		return f != 0;
	}
};

struct mxs_vector
{
	float x, y, z;
};

struct mxs_angvec
{
	float x, y, z;
};

class cScrVec
{
public:
	cScrVec ()
	{
		pos.x = 0;
		pos.y = 0;
		pos.z = 0;
	}
	cScrVec (float i, float j, float k)
	{
		pos.x = i;
		pos.y = j;
		pos.z = k;
	}
	cScrVec (const mxs_vector& rV)
	{
		pos = rV;
	}
	cScrVec& operator = (const mxs_vector& v)
	{
		pos = v;
		return *this;
	}
	operator const mxs_vector& () const
	{
		return pos;
	}
	operator mxs_vector& () 
	{
		return pos;
	}
	cScrVec operator + (const cScrVec& r) const
	{
		cScrVec ret;
		ret.pos = pos;
		ret.pos.x += r.pos.x;
		ret.pos.y += r.pos.y;
		ret.pos.z += r.pos.z;
		return ret;
	}
	cScrVec operator + (const mxs_vector& v) const
	{
		cScrVec ret;
		ret.pos = pos;
		ret.pos.x += v.x;
		ret.pos.y += v.y;
		ret.pos.z += v.z;
		return ret;
	}
	cScrVec& operator += (const cScrVec& r)
	{
		pos.x += r.pos.x;
		pos.y += r.pos.y;
		pos.z += r.pos.z;
		return *this;
	}
	cScrVec& operator += (const mxs_vector& v)
	{
		pos.x += v.x;
		pos.y += v.y;
		pos.z += v.z;
		return *this;
	}
	cScrVec operator - (const cScrVec& r) const
	{
		cScrVec ret;
		ret.pos = pos;
		ret.pos.x -= r.pos.x;
		ret.pos.y -= r.pos.y;
		ret.pos.z -= r.pos.z;
		return ret;
	}
	cScrVec operator - (const mxs_vector& v) const
	{
		cScrVec ret;
		ret.pos = pos;
		ret.pos.x -= v.x;
		ret.pos.y -= v.y;
		ret.pos.z -= v.z;
		return ret;
	}
	cScrVec& operator -= (const cScrVec& r)
	{
		pos.x -= r.pos.x;
		pos.y -= r.pos.y;
		pos.z -= r.pos.z;
		return *this;
	}
	cScrVec& operator -= (const mxs_vector& v)
	{
		pos.x -= v.x;
		pos.y -= v.y;
		pos.z -= v.z;
		return *this;
	}
	float Magnitude() const;
	float MagSquared() const;

	mxs_vector pos;
};


// A cScrStr should _never_ be NULL.
class cScrStr
{
	const char* m_pszData;

	static const char* _ChNil;
public:
	cScrStr () : m_pszData(_ChNil)
		{ }
	cScrStr (const char* psz) 
		{ m_pszData = IF_NOT(psz, _ChNil); }

	cScrStr& operator = (char* psz)
		{ m_pszData = IF_NOT(psz, _ChNil); return *this;}

	operator const char* () const
		{ return m_pszData; }

	// Normally, a cScrStr should be considered const
	// In a few cases, you may need to do this, however.
	// No checking is performed, so be absolutely sure when you call this.
	void Free();

	BOOL IsEmpty(void) const
		{ return (m_pszData == NULL) || (*m_pszData == 0); }
};

class cAnsiStr
{
	char*	m_pchData;
	int		m_nDataLength;
	int		m_nAllocLength;

	static const char* _ChNil;
public:
	~cAnsiStr();
	cAnsiStr();
	cAnsiStr(int);
	cAnsiStr(const char *);
	cAnsiStr(char);
	cAnsiStr(const char *, int);
	cAnsiStr(const cAnsiStr &);
	cAnsiStr(const cScrStr &);

	operator const char* () const
		{ return m_pchData; }

	cAnsiStr& operator= (const cAnsiStr& rCpy)
		{ Assign(rCpy.m_nDataLength, rCpy.m_pchData); return *this; }
	cAnsiStr& operator= (const char* psz);
	cAnsiStr& operator= (const cScrStr& rCpy)
		{ return *this = static_cast<const char*>(rCpy); }
	cAnsiStr& operator+= (const cAnsiStr& rCat)
		{ Append(rCat.m_nDataLength, rCat.m_pchData); return *this; }
	cAnsiStr& operator+= (const char* psz);
	cAnsiStr& operator+= (const cScrStr& rCat)
		{ return *this += static_cast<const char*>(rCat); }

	friend cAnsiStr operator+ (const cAnsiStr&, const cAnsiStr&);
	friend cAnsiStr operator+ (const cAnsiStr&, const char*);
	friend cAnsiStr operator+ (const char*, const cAnsiStr&);

	char* AllocStr(int);
	char* ReallocStr(char*, int);
	void FreeStr(char*);
	void AllocBuffer(int);

	void Attach(char*, int, int);
	char* Detach(void);
	void BufDone(int nLength, int nAlloc = -1);

	// Creates a new string with an allocated buffer of length + extra
	// and copies length characters from us beginning at start.
	void AllocCopy(cAnsiStr&, int length, int extra, int start) const;

	void Assign(int, const char *);
	void Append(int, const char *);
	void Append(char);
	void ConcatCopy(int, const char*, int, const char*);

	int Insert(const char *, int pos = 0);
	int Insert(char, int pos = 0);
	void Remove(int, int);
	void Empty(void);
	void Trim(void);

	int GetLength(void) const
		{ return m_nDataLength; }
	BOOL IsEmpty(void) const
		{ return m_nDataLength == 0; }
	BOOL IsInitialEmpty(void) const
		{ return m_pchData == _ChNil; }

	char GetAt(int pos) const
		{ return (pos < m_nDataLength) ? m_pchData[pos] : '\0'; }
	char SetAt(int pos, char ch)
		{ return (pos < m_nDataLength) ? m_pchData[pos] = ch : '\0'; }
	char operator [] (unsigned int pos) const
		{ return m_pchData[pos]; }

	int Compare(int, const char *) const;
	int Compare(const cAnsiStr& rStr) const
		{ return Compare(rStr.m_nDataLength, rStr.m_pchData); }
	int Compare(const char *pchData) const
		{ return Compare(strlen(pchData) + 1, pchData); }

	int Find(const char *, int start = 0) const;
	int Find(char, int start = 0) const;
	int ReverseFind(char) const;
	int FindOneOf(const char*, int start = 0) const;

	int SpanIncluding(const char*, int start = 0) const;
	int SpanExcluding(const char*, int start = 0) const;
	int ReverseIncluding(const char*, int start = 0) const;
	int ReverseExcluding(const char*, int start = 0) const;

	enum eQuoteMode
	{
		kOff,
		kDoubleQuotes,
		kEscapeQuotes,
		kQuoteIfWhite,
		kRemoveEmbeddedQuotes
	};
	
	cAnsiStr& Quoted(eQuoteMode);
	void FmtStr(unsigned int, const char*, ...);
	void FmtStr(const char*, ...);
};

cAnsiStr operator+ (const cAnsiStr&, const cAnsiStr&);
cAnsiStr operator+ (const cAnsiStr&, const char*);
cAnsiStr operator+ (const char*, const cAnsiStr&);
inline bool operator == (const cAnsiStr& lStr, const cAnsiStr& rStr)
	{ return lStr.Compare(rStr) == 0; }
inline bool operator == (const cAnsiStr& lStr, const char *rStr)
	{ return lStr.Compare(strlen(rStr) + 1, rStr) == 0; }
inline bool operator != (const cAnsiStr& lStr, const cAnsiStr& rStr)
	{ return lStr.Compare(rStr) != 0; }
inline bool operator != (const cAnsiStr& lStr, const char *rStr)
	{ return lStr.Compare(strlen(rStr) + 1, rStr) != 0; }

/*
	SetAt...
	Compare...
	CompareNoCase...
	Collate...
	Mid...
	MakeUpper...
	MakeLower...
	MakeReverse...
	void FmtStr(unsigned int, const char*, ...);
	void FmtStr(const char*, ...);
	void FmtStr(unsigned int, unsigned short, ...);
	void FmtStr(unsigned short, ...);
	char* GetBuffer(int);
	void ReleaseBuffer(void);
	GetBufferSetLength...
	BufIn
	BufOut
	BufInOut
not really sure why we need this
	void DoGrowBuffer(int);
	int ToStream(class cOStore&) const;
	int FromStream(class cIStore&);
	int LoadString(unsigned short);
*/


// Multi-parm type
enum eMultiParmType
{
	kMT_Undef,
	kMT_Int,
	kMT_Float,
	kMT_String,
	kMT_Vector,
	kMT_Boolean
};

struct sMultiParm
{
	union {
		int i;
		float  f;
		char*  psz;
		mxs_vector*  pVector;
		unsigned int b;
	};
	eMultiParmType type;
};

class cMultiParm : public sMultiParm
{
public:
	// There is no destructor. 
	// If the multiparm has an allocated pointer in it
	// (which happens when you assign a string or vector)
	// it will be leaked unless you explicitly free it.
	// I don't do it automatically so that
	// 'cMultiParm("string")' is a simple pointer copy.

	cMultiParm ()
		{ psz = NULL; type = kMT_Undef; }
	cMultiParm (int ival)
		{ i = ival; type = kMT_Int; }
	cMultiParm (long ival)
		{ i = ival; type = kMT_Int; }
	cMultiParm (short ival)
		{ i = ival; type = kMT_Int; }
	cMultiParm (float fval)
		{ f = fval; type = kMT_Float; }
	cMultiParm (double fval)
		{ f = fval; type = kMT_Float; }
	cMultiParm (bool bval)
		{ b = bval; type = kMT_Boolean; }
	cMultiParm (true_bool& bval)
		{ b = static_cast<bool>(bval); type = kMT_Boolean; }
	// Strings and vectors just copy the pointer.
	// They will be freed if you assign something else to the MP
	cMultiParm (char* pszval);
	cMultiParm (mxs_vector* pVectorval);
	cMultiParm (sMultiParm& rcpy);

	// Simple assignment methods. 
	// The old contents are clobbered without regard.
	// Not recommended if the MP may have an allocated string or vector.
	void Unset()
		{ type = kMT_Undef; }
	void Set(int ival)
		{ i = ival; type = kMT_Int; }
	void Set(float fval)
		{ f = fval; type = kMT_Float; }
	void Set(bool bval)
		{ b = bval; type = kMT_Boolean; }
	void Set(char* pszval)
		{ psz = pszval; type = kMT_String; }
	void Set(mxs_vector* pVectorval)
		{ pVector = pVectorval; type = kMT_Vector; }

	// Assignment will destroy a string or vector pointer.
	cMultiParm& operator = (int ival);
	cMultiParm& operator = (long ival)
		{ *this = static_cast<int>(ival); return *this; }
	cMultiParm& operator = (float fval);
	cMultiParm& operator = (double fval)
		{ *this = static_cast<float>(fval); return *this; }
	cMultiParm& operator = (const char* pszval);
	cMultiParm& operator = (const mxs_vector* pVectorval);
	cMultiParm& operator = (bool bval);
	cMultiParm& operator = (const true_bool& bval)
		{ *this = static_cast<bool>(bval); return *this; }
	// Assigning a MP "steals" the pointer.
	cMultiParm& operator = (sMultiParm& mp);

	operator int () const;
	operator float () const;
	operator bool () const;
	// These pointers are constant, don't fsck with them.
	operator const char* () const;
	operator const mxs_vector* () const;
	
	// Explicit destructor.
	void Free();
};


class object
{
public:
	operator int () const
		{ return id; }
	object& operator= (int i)
		{ id = i; return *this; }
	bool operator== (const object & o)
		{ return id == o.id; }
	bool operator== (int i)
		{ return id == i; }
	bool operator!= (const object & o)
		{ return id != o.id; }
	bool operator!= (int i)
		{ return id != i; }
	object (int iId = 0) : id(iId) { }
	int id;
};


interface ILinkQuery;

struct sLink
{
	int   source;
	int   dest;
	short flavor;
};

class link
{
public:
	/*
	object* To (object&, sLink *);
	cMultiParm* GetData (cMultiParm&, const char * field);
	void SetData (const char *, cMultiParm &);
	*/
	link (long i = 0) : id(i) { }
	link& operator= (long i)
		{ id = i; return *this; }
	operator long () const
		{ return id; }
	long id;
};

class linkkind
{
public:
	/*
	linkkind (linkkind);
	*/
	linkkind (int iId = 0) : id(iId) { }
	bool operator== (const linkkind & f)
		{ return id == f.id; }
	bool operator!= (const linkkind & f)
		{ return id != f.id; }
	operator long () const
		{ return id; }
	long id;
};

class linkset
{
public:
	linkset() : query(NULL) { }
	inline ~linkset();
	inline link* Link(link&) const;
	inline void NextLink();
	inline true_bool* AnyLinksLeft(true_bool&) const;

	ILinkQuery* query;
};

class reaction_kind
{
public:
	long id;
	reaction_kind (long iId = 0) : id(iId) { }
	operator long () const
		{ return id; }
};


interface IScript;
typedef IScript* (__cdecl *ScriptFactoryProc)(const char*, int);

struct sScrClassDesc
{
	const char* pszModule;
	const char* pszClass;
	const char* pszBaseClass;
	ScriptFactoryProc pfnFactory;
};

enum eScrTraceAction {
	kNoAction,
	kBreak,
	kSpew
};

DECLARE_HANDLE(tScrIter);


struct sScrDatumTag
{
	int  objId;
	const char *  pszClass;
	const char *  pszName;
};


enum eFieldType
{
	kFieldTypeInt = 0,
	kFieldTypeBoolean = 1,
	kFieldTypeUnsignedInt = 2,
	kFieldTypeBitVector = 3,
	kFieldTypeEnum = 4,
	kFieldTypeString = 5,
	kFieldTypeData = 7,
	kFieldTypeVector = 9,
	kFieldTypeFloat = 10
};

struct sFieldDesc
{
	char name[32];
	eFieldType type;
	unsigned long size;
	unsigned long offset;
	int  flags;
	int  min, max;
	int num_bits;
	const char* * bit_names;
};

struct sStructDesc
{
	char name[32];
	unsigned long size;
	int  zero;
	int  num_fields;
	sFieldDesc* fields;
};


struct sTraitDesc
{
};

struct sDispatchListenerDesc
{
};

struct sDispatchMsg
{
};

struct sHierarchyMsg
{
};

// AI uses this
struct sSoundInfo
{
};

#endif // _LG_TYPES_H
