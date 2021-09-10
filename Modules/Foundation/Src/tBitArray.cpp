// tBitArray.cpp
//
// A tBitArray is a holder for an arbitrary number of bits and allows individual access to each bit, the ability to
// clear or set all bits, and some simple binary bitwise operators such as 'and', 'xor', and 'or'. It currently does
// not support dynamic growing or shrinking.
//
// Comparisons
// tBitArray - Use when you want to store a large number of bits and you don't know how many at compile-time.
//             This type os primatily for storage and access to a large number of bits. Not many bitwise or
//             mathematical operators.
// tBitField - Use when know how many bits at compile-time and you want bitwise logic opertors like and, or, xor,
//             shift, not, etc. Good for storing a fixed number of flags or channels etc.
// tFixInt   - Use when you want full mathematical operations like any built-in integral type. Size must be known at
//             compile time and must be a multiple of 32 bits. You get + - / * etc as well as all bitwise logic ops.
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

#include "Foundation/tBitArray.h"
#include "Foundation/tStandard.h"


void tBitArray::Set(int numBits)
{
	Clear();
	tAssert(numBits > 0);

	NumBits = numBits;
	int n = GetNumElements();
	ElemData = new uint32[n];
	tStd::tMemset(ElemData, 0, n*sizeof(uint32));
}


void tBitArray::Set(const uint32* data, int numBits)
{
	Clear();
	tAssert(data && numBits);

	NumBits = numBits;
	int n = GetNumElements();
	ElemData = new uint32[n];
	tStd::tMemcpy(ElemData, data, n*sizeof(uint32));
	ClearPadBits();
}


void tBitArray::Set(const tBitArray& src)
{
	if (&src == this)
		return;

	Clear();
	if (!src.IsValid())
		return;

	NumBits = src.NumBits;
	int n = src.GetNumElements();
	ElemData = new uint32[n];
	tStd::tMemcpy(ElemData, src.ElemData, n*sizeof(uint32));
}


void tBitArray::InvertAll()
{
	int n = GetNumElements();
	for (int i = 0; i < n; i++)
		ElemData[i] = ~ElemData[i];

	ClearPadBits();
}


bool tBitArray::AreAll(bool v) const
{
	tAssert(ElemData);
	int n = GetNumElements();
	uint32 fullField = v ? 0xFFFFFFFF : 0x00000000;
	for (int i = 0; i < n-1; i++)
	{
		if (ElemData[i] != fullField)
			return false;
	}

	// Deal with the bits in the last field.
	int last = NumBits & 0x1F;
	uint32 maxFull = (last ? (1 << last) : 0) - 1;
	fullField = v ? maxFull : 0x00000000;
	return (ElemData[n-1] & maxFull) == fullField;
}


int tBitArray::CountBits(bool val) const
{
	// Uses technique described here "http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan".
	// This is one reason the pad bits must always be cleared.
	tAssert(ElemData);
	int numFields = GetNumElements();

	int count = 0;
	for (int n = 0; n < numFields; n++)
	{
		uint32 v = ElemData[n];			// Count the number of bits set in v.
		uint32 c;						// c accumulates the total bits set in v.
		for (c = 0; v; c++)
			v &= v - 1;					// Clear the least significant bit set.
		count += c;
	}
	
	return val ? count : (NumBits - count);
}


int tBitArray::GetClearedBit(int index) const
{
	tAssert(ElemData);

	// Find the first zero bit. The operation we do is log2((a xor (a+1)) +1)
	uint32 field = ElemData[index];

	// This is guaranteed to be a power of two.
	uint32 freeBit = (field ^ (field+1)) + 1;

	// Wraps around on the last bit.
	if (!freeBit)
		return 31;

	// Now get the log in base 2 of freeBit. See "http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog".
	const uint32 b[] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
	uint32 c = (freeBit & b[0]) != 0;

	c |= ((freeBit & b[4]) != 0) << 4;
	c |= ((freeBit & b[3]) != 0) << 3;
	c |= ((freeBit & b[2]) != 0) << 2;
	c |= ((freeBit & b[1]) != 0) << 1;

	// This is the first cleared index in the bit array.
	return c-1;
}


int tBitArray::GetClearedBitPos() const
{
	int n = GetNumElements();

	for (int i = 0; i < n-1; i++)
	{
		if (ElemData[i] < 0xFFFFFFFF)
			return 32*i + GetClearedBit(i);
	}

	int last = NumBits & 0x1F;
	uint32 maxFull = (last ? (1 << last) : 0) - 1;

	if (ElemData[n-1] < maxFull)
		return 32*(n-1) + GetClearedBit(n-1);

	// There are no free bits available.
	return -1;
}


tBitArray& tBitArray::operator&=(const tBitArray& s)
{
	tAssert(NumBits == s.NumBits);
	int n = GetNumElements();
	for (int i = 0; i < n; i++)
		ElemData[i] &= s.ElemData[i];

	return *this;	// No need to ensure pad bits are cleared because 0 & 0 = 0.
}


tBitArray& tBitArray::operator|=(const tBitArray& s)
{
	tAssert(NumBits == s.NumBits);
	int n = GetNumElements();
	for (int i = 0; i < n; i++)
		ElemData[i] |= s.ElemData[i];

	return *this;	// No need to ensure pad bits are cleared because 0 | 0 = 0.
}


tBitArray& tBitArray::operator^=(const tBitArray& s)
{
	tAssert(NumBits == s.NumBits);
	int n = GetNumElements();
	for (int i = 0; i < n; i++)
		ElemData[i] ^= s.ElemData[i];

	return *this;	// No need to ensure pad bits are cleared because 0 ^ 0 = 0.
}
