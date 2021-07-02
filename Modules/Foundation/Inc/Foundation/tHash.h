// tHash.h
//
// Hash functions for various kinds of data. Using 64 or 256 bit versions if you want to avoid collisions. There are two
// 32 bit hash functions. A fast version used for most string hashes, and a slower but better version. All functions
// return the supplied initialization vector(iv) if there was no data to hash. To compute a single hash from multiple
// data sources like strings, binary data, or files, you do NOT need to consolidate all the source data into one buffer
// first. Just set the initialization vector to the hash computed from the previous step.
//
// Copyright (c) 2004-2006, 2015, 2017, 2019, 2021 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.
//
// The SHA-256 implementation is taken from https://github.com/amosnier/sha-2. All functions and constants in the
// tHash_SHA256 namespace should be considered unencumbered as per Alain Mosnier's licence file:
//
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or distribute this software, either in source code form
// or as a compiled binary, for any purpose, commercial or non-commercial, and by any means.
//
// In jurisdictions that recognize copyright laws, the author or authors of this software dedicate any and all copyright
// interest in the software to the public domain. We make this dedication for the benefit of the public at large and to
// the detriment of our heirs and successors. We intend this dedication to be an overt act of relinquishment in
// perpetuity of all present and future rights to this software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <http://unlicense.org>

#pragma once
#include <Foundation/tStandard.h>
#include <Foundation/tString.h>
#include <Foundation/tFixInt.h>
namespace tHash
{


enum class tHashAlgorithm
{
	Fast32,
	Jenkins32,
	Jenkins64,
	MD5,													// MD5 is 128 bit. For cryptographic purposes, no MD5.
	Jenkins256,
	SHA256
};


// These initialization vectors should not be modified unless you want to break a lot of code. The zero 32bit one is
// responsible for things like stringhash returning zero on empty strings.
const uint32 HashIV32 = 0;
const uint64 HashIV64 = 0;
const tuint128 HashIV128 = 0;
const tuint256 HashIV256 = 0;

// Normally the initial iv should not be modified for these fast hash functions. The fast hash functions are the only
// ones that store the entire state in the hash. Allows you to concatenate hashes of separate strings/data-sequences
// together by passing the hash of the previous call into the function again. This way you don't need to create a
// concatenated string/data-set to get its hash, you simply chain multiple calls together. The fast hash functions
// are the only ones that guarantee the same hash value whether computed in parts or as a single data-set.
uint32 tHashDataFast32(const uint8* data, int length, uint32 iv = HashIV32);
uint32 tHashStringFast32(const char*, uint32 iv = HashIV32);
uint32 tHashStringFast32(const tString&, uint32 iv = HashIV32);
uint32 tHashString(const char*);

// The CT (Compile Time) variant uses the fast-hash algorithm. It is super handy for use in the 'case' part of switch
// statements or any time you know the string literal explicitly. In these cases the compiler can do all the work.
constexpr uint32 tHashCT(const char*, uint32 iv = HashIV32);

// The HashData32/64/128/256 and variants do _not_ guarantee the same hash value if they are chained together compared
// to the hash of the same data computed as a single block. This is because the entire state is not stored in the hash
// itself since these are much better hash functions than the Fast32 versions. Chaining is still useful as uniqueness is
// still guaranteed and if any data changes in any of the sources the end result will vary. Chaining is performed in the
// same manner as HashDataFast32. Algorithms in use for below functions:
//
// tHash*32:  Robert J. Jenkins Jr., 1997. See http://burtleburtle.net/bob/hash/evahash.html
// tHash*64:  Robert J. Jenkins Jr., 1997. See http://burtleburtle.net/bob/hash/evahash.html
// tHash*128: MD5. Not cryptographically secure any more.
// tHash*256: Robert J. Jenkins Jr., 1997. See http://burtleburtle.net/bob/hash/evahash.html
//
// If you want SHA-256 call it directly.
uint32 tHashData32(const uint8* data, int length, uint32 iv = HashIV32);
uint32 tHashString32(const char*, uint32 iv = HashIV32);
uint32 tHashString32(const tString&, uint32 iv = HashIV32);

uint64 tHashData64(const uint8* data, int length, uint64 iv = HashIV64);
uint64 tHashString64(const char*, uint64 iv = HashIV64);
uint64 tHashString64(const tString&, uint64 iv = HashIV64);

tuint128 tHashData128(const uint8* data, int length, tuint128 iv = HashIV128);
tuint128 tHashString128(const char*, tuint128 iv = HashIV128);
tuint128 tHashString128(const tString&, tuint128 iv = HashIV128);

tuint256 tHashData256(const uint8* data, int length, tuint256 iv = HashIV256);
tuint256 tHashString256(const char*, const tuint256& iv = HashIV256);
tuint256 tHashString256(const tString&, const tuint256& iv = HashIV256);

// The MD5 functions are used by the HashData128 functions. MD5 is _not_ to be used for cryptographic purposes.
// For reference and testing:
// MD5("The quick brown fox jumps over the lazy dog") = 9e107d9d372bb6826bd81d3542a419d6
// MD5("The quick brown fox jumps over the lazy dog.") = e4d909c290d0fb1ca068ffaddf22cbd0
tuint128 tHashDataMD5(const uint8* data, int length, tuint128 iv = HashIV128);
tuint128 tHashStringMD5(const char*, tuint128 iv = HashIV128);
tuint128 tHashStringMD5(const tString&, tuint128 iv = HashIV128);

tuint256 tHashDataSHA256(const uint8* data, int length, tuint256 iv = HashIV256);
tuint256 tHashStringSHA256(const char*, tuint256 iv = HashIV256);
tuint256 tHashStringSHA256(const tString&, tuint256 iv = HashIV256);


// Implementation below this line.


inline uint32 tHashStringFast32(const char* string, uint32 iv)
{
	if (!string)
		return 0;
	return tHashDataFast32((uint8*)string, tStd::tStrlen(string), iv);
}


// This (compile-time) constant expression relies on the odometer-style looping of unsigned ints to compute the hash.
// Since it's inline, you may need to pragma warning(disable:4307), which warns of const integral overflow.
inline constexpr uint32 tHashCT(const char* s, uint32 hash)																{ return *s ? tHashCT(s + 1, hash + (hash << 5) + uint8(*s)) : hash; }
inline uint32 tHashStringFast32(const tString& s, uint32 iv)															{ return tHashStringFast32(s.ConstText(), iv); }
inline uint32 tHashString(const char* s)																				{ return tHashStringFast32(s); }
inline uint32 tHashString32(const char* string, uint32 iv)																{ return tHashData32((uint8*)string, tStd::tStrlen(string), iv); }
inline uint32 tHashString32(const tString& s, uint32 iv)																{ return tHashString32(s.ConstText(), iv); }
inline uint64 tHashString64(const char* string, uint64 iv)																{ return tHashData64((uint8*)string, tStd::tStrlen(string), iv); }
inline uint64 tHashString64(const tString& s, uint64 iv)																{ return tHashString64(s.ConstText(), iv); }
inline tuint128 tHashStringMD5(const char* string, tuint128 iv)															{ return tHashDataMD5((uint8*)string, tStd::tStrlen(string), iv); }
inline tuint128 tHashStringMD5(const tString& s, tuint128 iv)															{ return tHashStringMD5(s.ConstText(), iv); }
inline tuint128 tHashData128(const uint8* data, int length, tuint128 iv)												{ return tHashDataMD5(data, length, iv); }
inline tuint128 tHashString128(const char* string, tuint128 iv)															{ return tHashDataMD5((uint8*)string, tStd::tStrlen(string), iv); }
inline tuint128 tHashString128(const tString& s, tuint128 iv)															{ return tHashStringMD5(s.ConstText(), iv); }
inline tuint256 tHashString256(const char* string, const tuint256& iv)													{ return tHashData256((uint8*)string, tStd::tStrlen(string), iv); }
inline tuint256 tHashString256(const tString& s, const tuint256& iv)													{ return tHashString256(s.ConstText(), iv); }


}
