#pragma once

#include <stdint.h>

typedef uint8_t uint8;
typedef int8_t sint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t sint16;
typedef int16_t int16;
typedef uint32_t uint32;
typedef int32_t sint32;
typedef int32_t int32;
typedef uint64_t uint64;
typedef int64_t sint64;
typedef int64_t int64;

typedef uint8* uint8_ptr;
typedef sint8* sint8_ptr;
typedef int8* int8_ptr;
typedef uint16* uint16_ptr;
typedef sint16* sint16_ptr;
typedef int16* int16_ptr;
typedef uint32* uint32_ptr;
typedef sint32* sint32_ptr;
typedef int32* int32_ptr;
typedef uint64* uint64_ptr;
typedef sint64* sint64_ptr;
typedef int64* int64_ptr;

typedef uint8 const* uint8_const_ptr;
typedef sint8 const* sint8_const_ptr;
typedef int8 const* int8_const_ptr;
typedef uint16 const* uint16_const_ptr;
typedef sint16 const* sint16_const_ptr;
typedef int16 const* int16_const_ptr;
typedef uint32 const* uint32_const_ptr;
typedef sint32 const* sint32_const_ptr;
typedef int32 const* int32_const_ptr;
typedef uint64 const* uint64_const_ptr;
typedef sint64 const* sint64_const_ptr;
typedef int64 const* int64_const_ptr;

const uint8 max_uint8 = 0xFFU;
const sint8 min_int8 = sint8(0x80);
const sint8 max_int8 = 0x7F;
const uint16 max_uint16 = 0xFFFFU;
const sint16 min_int16 = sint16(0x8000);
const sint16 max_int16 = 0x7FFF;
const uint32 max_uint32 = 0xFFFFFFFFU;
const sint32 min_int32 = 0x80000000;
const sint32 max_int32 = 0x7FFFFFFF;
const uint64 max_uint64 = 0xFFFFFFFFFFFFFFFFULL;
const sint64 min_int64 = 0x8000000000000000LL;
const sint64 max_int64 = 0x7FFFFFFFFFFFFFFFLL;

template<int N> struct UIntTraits {};
template<> struct UIntTraits<8> { typedef uint8 Type; };
template<> struct UIntTraits<16> { typedef uint16 Type; };
template<> struct UIntTraits<32> { typedef uint32 Type; };
template<> struct UIntTraits<64> { typedef uint64 Type; };
template<int N> using UInt = typename UIntTraits<N>::Type;
