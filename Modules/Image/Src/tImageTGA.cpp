// tImageTGA.cpp
//
// This class knows how to load and save targa (.tga) files into tPixel arrays. These tPixels may be 'stolen' by the
// tPicture's constructor if a targa file is specified. After the array is stolen the tImageTGA is invalid. This is
// purely for performance.
//
// Copyright (c) 2006, 2017, 2019, 2020, 2023, 2024 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <System/tFile.h>
#include "Image/tImageTGA.h"
#include "Image/tPicture.h"
using namespace tSystem;
namespace tImage
{


bool tImageTGA::Load(const tString& tgaFile)
{
	Clear();

	if (tSystem::tGetFileType(tgaFile) != tSystem::tFileType::TGA)
		return false;

	if (!tFileExists(tgaFile))
		return false;

	int numBytes = 0;
	uint8* tgaFileInMemory = tLoadFile(tgaFile, nullptr, &numBytes);
	bool success = Load(tgaFileInMemory, numBytes);
	delete[] tgaFileInMemory;

	return success;
}


bool tImageTGA::Load(const uint8* tgaFileInMemory, int numBytes)
{
	Clear();
	if ((numBytes <= 0) || !tgaFileInMemory)
		return false;

	#pragma pack(push, r1, 1)
	struct TGAHeader
	{
		int8 IDLength;
		int8 ColourMapType;
		int8 DataTypeCode;
		int16 ColourMapOrigin;
		int16 ColourMapLength;
		int8 ColourMapDepth;
		int16 OriginX;
		int16 OriginY;

		int16 Width;
		int16 Height;
		int8 BitDepth;

		// If Bit 5 of orientation is set the image will be upside down (like BMP).
		int8 Orientation;
	};
	#pragma pack(pop, r1)
	tStaticAssert(sizeof(TGAHeader) == 18);

	// Safety for corrupt files that aren't even as big as the tga header.
	// Checks later on will ensure data size is sufficient.
	if (numBytes < sizeof(TGAHeader))
		return false;

	TGAHeader* header = (TGAHeader*)tgaFileInMemory;
	Width = header->Width;
	Height = header->Height;
	int bitDepth = header->BitDepth;
	int dataType = header->DataTypeCode;

	// We support 16, 24, and 32 bit depths. We support data type mode 2 (uncompressed RGB) and mode 10 (Run-length
	// encoded RLE RGB). We allow a colour map to be present, but don't use it.
	if
	(
		((bitDepth != 16) && (bitDepth != 24) && (bitDepth != 32)) ||
		((dataType != 2) && (dataType != 10)) ||
		((header->ColourMapType != 0) && (header->ColourMapType != 1))
	)
	{
		Clear();
		return false;
	}
	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	if (bitDepth == 16)
		PixelFormatSrc = tPixelFormat::G3B5A1R5G2;
	else if (bitDepth == 24)
		PixelFormatSrc = tPixelFormat::R8G8B8;

	// The +1 adds sizeof header bytes.
	uint8* srcData = (uint8*)(header + 1);

	// These usually are zero. In most cases the pixel data will follow directly after the header. iColourMapType is a
	// boolean 0 or 1.
	srcData += header->IDLength;
	srcData += header->ColourMapType * header->ColourMapLength;
	const uint8* endData = tgaFileInMemory + numBytes;

	int numPixels = Width * Height;
	Pixels = new tPixel4[numPixels];

	// Read the image data.
	int bytesPerPixel = bitDepth >> 3;
	int pixel = 0;

	while (pixel < numPixels)
	{
		switch (dataType)
		{
			case 10:
			{
				// Safety for corrupt tga files that don't have enough data.
				int available = endData - srcData;
				if (available < bytesPerPixel+1)
				{
					Clear();
					return false;
				}

				// Image data is compressed.
				int j = srcData[0] & 0x7f;
				uint8 rleChunk = srcData[0] & 0x80;
				srcData += 1;

				tColour4b firstColour;
				ReadColourBytes(firstColour, srcData, bytesPerPixel);
				Pixels[pixel] = firstColour;
				pixel++;
				srcData += bytesPerPixel;

				if (rleChunk)
				{
					// Chunk is run length encoded.
					for (int i = 0; i < j; i++)
					{
						Pixels[pixel] = firstColour;
						pixel++;
					}
				}
				else
				{
					// Safety for corrupt tga files that don't have enough data.
					available = endData - srcData;
					if (available < bytesPerPixel*j)
					{
						Clear();
						return false;
					}

					// Chunk is normal.
					for (int i = 0; i < j; i++)
					{
						ReadColourBytes(Pixels[pixel], srcData, bytesPerPixel);
						pixel++;
						srcData += bytesPerPixel;
					}
				}
				break;
			}

			case 2:
			default:
			{
				// Safety for corrupt tga files that don't have enough data.
				int available = endData - srcData;
				if (available < bytesPerPixel)
				{
					Clear();
					return false;
				}

				// Not compressed.
				ReadColourBytes(Pixels[pixel], srcData, bytesPerPixel);
				pixel++;
				srcData += bytesPerPixel;
				break;
			}
		}
	}

	return true;
}


void tImageTGA::ReadColourBytes(tColour4b& dest, const uint8* src, int bytesPerPixel)
{
	switch (bytesPerPixel)
	{
		case 4:
			dest.R = src[2];
			dest.G = src[1];
			dest.B = src[0];
			dest.A = src[3];
			break;

		case 3:
			dest.R = src[2];
			dest.G = src[1];
			dest.B = src[0];
			dest.A = 0xFF;
			break;

		case 2:
			dest.R = (src[1] & 0x7c) << 1;
			dest.G = ((src[1] & 0x03) << 6) | ((src[0] & 0xe0) >> 2);
			dest.B = (src[0] & 0x1f) << 3;
			dest.A = (src[1] & 0x80);
			break;

		default:
			dest.MakeBlack();
			break;
	}
}


bool tImageTGA::Set(tPixel4* pixels, int width, int height, bool steal)
{
	Clear();
	if (!pixels || (width <= 0) || (height <= 0))
		return false;

	Width = width;
	Height = height;

	if (steal)
	{
		Pixels = pixels;
	}
	else
	{
		Pixels = new tPixel4[Width*Height];
		tStd::tMemcpy(Pixels, pixels, Width*Height*sizeof(tPixel4));
	}

	PixelFormatSrc = tPixelFormat::R8G8B8A8;
	return true;
}


bool tImageTGA::Set(tFrame* frame, bool steal)
{
	Clear();
	if (!frame || !frame->IsValid())
		return false;

	Set(frame->GetPixels(steal), frame->Width, frame->Height, steal);
	if (steal)
		delete frame;

	return true;
}


bool tImageTGA::Set(tPicture& picture, bool steal)
{
	Clear();
	if (!picture.IsValid())
		return false;

	tPixel4* pixels = steal ? picture.StealPixels() : picture.GetPixels();
	return Set(pixels, picture.GetWidth(), picture.GetHeight(), steal);
}


tFrame* tImageTGA::GetFrame(bool steal)
{
	if (!IsValid())
		return nullptr;

	tFrame* frame = new tFrame();
	frame->PixelFormatSrc = PixelFormatSrc;

	if (steal)
	{
		frame->StealFrom(Pixels, Width, Height);
		Pixels = nullptr;
	}
	else
	{
		frame->Set(Pixels, Width, Height);
	}
	
	return frame;
}


tImageTGA::tFormat tImageTGA::Save(const tString& tgaFile, tFormat format, tCompression compression) const
{
	SaveParams params;
	params.Format = format;
	params.Compression = compression;
	return Save(tgaFile, params);
}


tImageTGA::tFormat tImageTGA::Save(const tString& tgaFile, const SaveParams& params) const
{
	tFormat format = params.Format;
	if (!IsValid() || (format == tFormat::Invalid))
		return tFormat::Invalid;

	if (tSystem::tGetFileType(tgaFile) != tSystem::tFileType::TGA)
		return tFormat::Invalid;

	if (format == tFormat::Auto)
	{
		if (IsOpaque())
			format = tFormat::BPP24;
		else
			format = tFormat::BPP32;
	}

	bool success = false;
	switch (params.Compression)
	{
		case tCompression::None:
			success = SaveUncompressed(tgaFile, format);
			break;

		case tCompression::RLE:
			success = SaveCompressed(tgaFile, format);
			break;
	}

	if (!success)
		return tFormat::Invalid;

	return format;
}


bool tImageTGA::SaveUncompressed(const tString& tgaFile, tFormat format) const
{
	if ((format != tFormat::BPP24) && (format != tFormat::BPP32))
		return false;
	
	tFileHandle file = tOpenFile(tgaFile.Chr(), "wb");
	if (!file)
		return false;

	uint8 bitDepth = (format == tFormat::BPP24) ? 24 : 32;

	// imageDesc has the following important fields:
	// Bits 0-3:	Number of attribute bits associated with each pixel. For a 16bit image, this would be 0 or 1. For a
	//				24-bit image, it should be 0. For a 32-bit image, it should be 8.
	// Bit 5:		Orientation. If set, the image is upside down.
	uint8 imageDesc = 0x00;
	imageDesc |= (bitDepth == 24) ? 0 : 8;

	// We'll be writing a 24 or 32bit uncompressed tga.
	tPutc(0, file);									// ID string length.
	tPutc(0, file);									// Colour map type.
	tPutc(2, file);									// 2 = Uncompressed True Colour (2=true colour + no compression bit). Not palletized.
	tPutc(0, file); tPutc(0, file);
	tPutc(0, file); tPutc(0, file);
	tPutc(0, file);
	tPutc(0, file); tPutc(0, file);					// X origin.
	tPutc(0, file); tPutc(0, file);					// Y origin.
	uint16 w = Width;
	uint16 h = Height;
	tPutc((w & 0x00FF), file);						// Width.
	tPutc((w & 0xFF00) >> 8, file);
	tPutc((h & 0x00FF), file);						// Height.
	tPutc((h & 0xFF00) >> 8, file);
	tPutc(bitDepth, file);							// 24 or 32 bit depth.  RGB or RGBA.
	tPutc(imageDesc, file);							// Image desc.  See above.

	// If we had a non-zero ID string length, we'd write length characters here.
	int numPixels = Width*Height;
	for (int p = 0; p < numPixels; p++)
	{
		tPixel4& pixel = Pixels[p];
		tPutc(pixel.B, file);
		tPutc(pixel.G, file);
		tPutc(pixel.R, file);

		if (format == tFormat::BPP32)
			tPutc(pixel.A, file);
	}

	tCloseFile(file);
	return true;
}


bool tImageTGA::SaveCompressed(const tString& tgaFile, tFormat format) const
{
	if ((format != tFormat::BPP24) && (format != tFormat::BPP32))
		return false;

	// Open the file.
	tFileHandle file = tOpenFile(tgaFile.Chr(), "wb");
	if (!file)
		return false;

	uint8 bitDepth = (format == tFormat::BPP24) ? 24 : 32;
	int bytesPerPixel = bitDepth / 8;

	// imageDesc has the following important fields:
	// Bits 0-3:	Number of attribute bits associated with each pixel. For a 16bit image, this would be 0 or 1. For a
	//				24-bit image, it should be 0. For a 32-bit image, it should be 8.
	// Bit 5:		Orientation. If set, the image is upside down.
	uint8 imageDesc = 0;
	imageDesc |= (bitDepth == 24) ? 0 : 8;

	// We'll be writing a 24 or 32bit compressed tga.
	tPutc(0, file);									// ID string length.
	tPutc(0, file);									// Colour map type.
	tPutc(10, file);								// 10 = RLE Compressed True Colour (2=true colour + 8=RLE). Not palletized.
	tPutc(0, file); tPutc(0, file);
	tPutc(0, file); tPutc(0, file);
	tPutc(0, file);

	tPutc(0, file); tPutc(0, file);					// X origin.
	tPutc(0, file); tPutc(0, file);					// Y origin.
	uint16 w = Width;
	uint16 h = Height;
	tPutc((w & 0x00FF), file);						// Width.
	tPutc((w & 0xFF00) >> 8, file);
	tPutc((h & 0x00FF), file);						// Height.
	tPutc((h & 0xFF00) >> 8, file);

	tPutc(bitDepth, file);							// 24 or 32 bit depth. RGB or RGBA.
	tPutc(imageDesc, file);							// Image desc.  See above.

	int numPixels = Height * Width;
	int index = 0;
	uint32 colour = 0;
	uint32* chunkBuffer = new uint32[128];

	// Now we write the pixel packets.  Each packet is either raw or rle.
	while (index < numPixels)
	{
		bool rlePacket = false;
		tPixel4& pixelColour = Pixels[index];

		// Note that we process alphas as zeros if we are writing 24bits only. This ensures the colour comparisons work
		// properly -- we ignore alpha. Zero is used because the uint32 colour values are initialized to all 0s.
		uint8 alpha = (bytesPerPixel == 4) ? pixelColour.A : 0;
		colour = pixelColour.B + (pixelColour.G << 8) + (pixelColour.R << 16) + (alpha << 24);

		chunkBuffer[0] = colour;
		int rleCount = 1;

		// We try to find repeating bytes with a minimum length of 2 pixels. Maximum repeating chunk size is 128 pixels
		// as the first bit of the count is used for the packet type.
		while (index + rleCount < numPixels)
		{
			tPixel4& nextPixelColour = Pixels[index+rleCount];
			uint8 alp = (bytesPerPixel == 4) ? nextPixelColour.A : 0;
			uint32 nextCol = nextPixelColour.B + (nextPixelColour.G << 8) + (nextPixelColour.R << 16) + (alp << 24);

			if (colour != nextCol || rleCount == 128)
			{
				rlePacket = (rleCount > 1) ? true : false;
				break;
			}
			rleCount++;
		}

		if (rlePacket)
		{
			tPutc(128 | (rleCount - 1), file);
			tWriteFile(file, &colour, bytesPerPixel);
		}
		else
		{
			rleCount = 1;
			while (index + rleCount < numPixels)
			{
				tPixel4& nextPixelColour = Pixels[index+rleCount];
				uint8 alp = (bytesPerPixel == 4) ? nextPixelColour.A : 0;
				uint32 nextCol = nextPixelColour.B + (nextPixelColour.G << 8) + (nextPixelColour.R << 16) + (alp << 24);

				if ((colour != nextCol && rleCount < 128) || rleCount < 3)
				{
					chunkBuffer[rleCount] = colour = nextCol;
				}
				else
				{
					// Check if the exit condition was the start of a repeating colour.
					if (colour == nextCol)
						rleCount -= 2;
					break;
				}
				rleCount++;
			}

			// Write the raw packet data.
			tPutc(rleCount - 1, file);
			for (int i = 0; i < rleCount; i++)
			{
				colour = chunkBuffer[i];
				tWriteFile(file, &colour, bytesPerPixel);
			}
		}
		index += rleCount;
	}

	delete[] chunkBuffer;
	tCloseFile(file);
	return true;
}


bool tImageTGA::IsOpaque() const
{
	for (int p = 0; p < (Width*Height); p++)
	{
		if (Pixels[p].A < 255)
			return false;
	}

	return true;
}


tPixel4* tImageTGA::StealPixels()
{
	tPixel4* pixels = Pixels;
	Pixels = nullptr;
	Width = 0;
	Height = 0;
	return pixels;
}


}
