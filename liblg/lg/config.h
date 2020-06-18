/**************************
 * portability macros
 */

#ifndef _LG_CONFIG_H
#define _LG_CONFIG_H

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef _MSC_VER

// "__thiscall" is not, as I was led to believe, valid syntax.
// Just have to rely on it being the default.
#define __thiscall 

#define stricmp _stricmp

#endif // _MSC_VER

#ifdef __GNUC__

#define IF_NOT(a,b)	((a)?:(b))

#define stricmp strcasecmp

#else // !__GNUC__

#ifndef __attribute__
#define __attribute__(x)	
#endif

#define IF_NOT(a,b)	((a)?(a):(b))

#endif // !__GNUC__

#ifdef __BORLANDC__
#endif

#endif // _LG_CONFIG_H
