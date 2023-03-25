/*
xxHash - Fast Hash algorithm
Copyright (C) 2012-2014, Yann Collet.
BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

You can contact the author at :
- xxHash source repository : http://code.google.com/p/xxhash/
*/


//**************************************
// Tuning parameters
//**************************************
// Unaligned memory access is automatically enabled for "common" CPU, such as x86.
// For others CPU, the compiler will be more cautious, and insert extra code to ensure aligned access is respected.
// If you know your target CPU supports unaligned memory access, you want to force this option manually to improve performance.
// You can also enable this parameter if you know your input data will always be aligned (boundaries of 4, for U32).
#if defined(__ARM_FEATURE_UNALIGNED) || defined(__i386) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
#  define XXH_USE_UNALIGNED_ACCESS 1
#endif

// XXH_ACCEPT_NULL_INPUT_POINTER :
// If the input pointer is a null pointer, xxHash default behavior is to trigger a memory access error, since it is a bad pointer.
// When this option is enabled, xxHash output for null input pointers will be the same as a null-length input.
// This option has a very small performance cost (only measurable on small inputs).
// By default, this option is disabled. To enable it, uncomment below define :
//#define XXH_ACCEPT_NULL_INPUT_POINTER 1

// XXH_FORCE_NATIVE_FORMAT :
// By default, xxHash library provides endian-independant Hash values, based on little-endian convention.
// Results are therefore identical for little-endian and big-endian CPU.
// This comes at a performance cost for big-endian CPU, since some swapping is required to emulate little-endian format.
// Should endian-independance be of no importance for your application, you may set the #define below to 1.
// It will improve speed for Big-endian CPU.
// This option has no impact on Little_Endian CPU.
#define XXH_FORCE_NATIVE_FORMAT 0


//**************************************
// Compiler Specific Options
//**************************************
// Disable some Visual warning messages
#ifdef _MSC_VER  // Visual Studio
#  pragma warning(disable : 4127)      // disable: C4127: conditional expression is constant
#endif

#ifdef _MSC_VER    // Visual Studio
#  define FORCE_INLINE static __forceinline
#else 
#  ifdef __GNUC__
#    define FORCE_INLINE static inline __attribute__((always_inline))
#  else
#    define FORCE_INLINE static inline
#  endif
#endif


//**************************************
// Includes & Memory related functions
//**************************************
#include "xxhash.h"
// Modify the local functions below should you wish to use some other memory related routines
// for malloc(), free()
#include <stdlib.h>
FORCE_INLINE void* XXH_malloc(size_t s) { return malloc(s); }
FORCE_INLINE void  XXH_free  (void* p)  { free(p); }
// for memcpy()
#include <string.h>
FORCE_INLINE void* XXH_memcpy(void* dest, const void* src, size_t size) { return memcpy(dest,src,size); }


//**************************************
// Basic Types
//**************************************
#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   // C99
# include <stdint.h>
  typedef uint8_t  BYTE;
  typedef uint16_t U16;
  typedef uint32_t U32;
  typedef  int32_t S32;
  typedef uint64_t U64;
#else
  typedef unsigned char      BYTE;
  typedef unsigned short     U16;
  typedef unsigned int       U32;
  typedef   signed int       S32;
  typedef unsigned long long U64;
#endif

#if defined(__GNUC__)  && !defined(XXH_USE_UNALIGNED_ACCESS)
#  define _PACKED_XXH __attribute__ ((packed))
#else
#  define _PACKED_XXH
#endif

#if !defined(XXH_USE_UNALIGNED_ACCESS) && !defined(__GNUC__)
#  ifdef __IBMC__
#    pragma pack(1)
#  else
#    pragma pack(push, 1)
#  endif
#endif


typedef struct _U32_S { U32 v; } _PACKED_XXH U32_S_XXH;

#if !defined(XXH_USE_UNALIGNED_ACCESS) && !defined(__GNUC__)
#  pragma pack(pop)
#endif

#define A32_XXH(x) (((U32_S_XXH *)(x))->v)


//***************************************
// Compiler-specific Functions and Macros
//***************************************
#define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)

// Note : although _rotl exists for minGW (GCC under windows), perform