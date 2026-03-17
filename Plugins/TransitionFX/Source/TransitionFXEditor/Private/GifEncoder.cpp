// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "GifEncoder.h"
#include "Misc/FileHelper.h"

// ─────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────

FGifEncoder::FGifEncoder(int32 InWidth, int32 InHeight, int32 InFrameDelay)
	: Width(InWidth)
	, Height(InHeight)
	, FrameDelay(InFrameDelay)
{
}

void FGifEncoder::AddFrame(const TArray<FColor>& Pixels)
{
	check(Pixels.Num() == Width * Height);
	Frames.Add(Pixels);
}

// ─────────────────────────────────────────────
// Color quantization — median-cut
// ─────────────────────────────────────────────

void FGifEncoder::FColorBox::ComputeStats()
{
	if (Colors.Num() == 0)
	{
		Range = 0;
		Channel = 0;
		Average = FColor::Black;
		return;
	}

	uint8 MinR = 255, MaxR = 0;
	uint8 MinG = 255, MaxG = 0;
	uint8 MinB = 255, MaxB = 0;
	uint32 SumR = 0, SumG = 0, SumB = 0;

	for (const FColor& C : Colors)
	{
		MinR = FMath::Min(MinR, C.R); MaxR = FMath::Max(MaxR, C.R);
		MinG = FMath::Min(MinG, C.G); MaxG = FMath::Max(MaxG, C.G);
		MinB = FMath::Min(MinB, C.B); MaxB = FMath::Max(MaxB, C.B);
		SumR += C.R; SumG += C.G; SumB += C.B;
	}

	int32 RangeR = MaxR - MinR;
	int32 RangeG = MaxG - MinG;
	int32 RangeB = MaxB - MinB;

	if (RangeR >= RangeG && RangeR >= RangeB)
	{
		Channel = 0;
		Range = RangeR;
	}
	else if (RangeG >= RangeR && RangeG >= RangeB)
	{
		Channel = 1;
		Range = RangeG;
	}
	else
	{
		Channel = 2;
		Range = RangeB;
	}

	int32 N = Colors.Num();
	Average = FColor(
		static_cast<uint8>(SumR / N),
		static_cast<uint8>(SumG / N),
		static_cast<uint8>(SumB / N),
		255);
}

void FGifEncoder::BuildGlobalPalette()
{
	// Sample colors from all frames (uniform sampling to keep memory reasonable)
	constexpr int32 MaxSamples = 100000;
	int32 TotalPixels = Frames.Num() * Width * Height;
	int32 Step = FMath::Max(1, TotalPixels / MaxSamples);

	TArray<FColor> SampledColors;
	SampledColors.Reserve(FMath::Min(TotalPixels, MaxSamples));

	int32 Counter = 0;
	for (const TArray<FColor>& Frame : Frames)
	{
		for (const FColor& C : Frame)
		{
			if (Counter % Step == 0)
			{
				SampledColors.Add(FColor(C.R, C.G, C.B, 255));
			}
			Counter++;
		}
	}

	// Median-cut: start with one box containing all sampled colors
	TArray<FColorBox> Boxes;
	{
		FColorBox InitialBox;
		InitialBox.Colors = MoveTemp(SampledColors);
		InitialBox.ComputeStats();
		Boxes.Add(MoveTemp(InitialBox));
	}

	// Split until we have 256 boxes (or can't split further)
	while (Boxes.Num() < 256)
	{
		// Find the box with the largest range
		int32 BestIdx = -1;
		int32 BestRange = 0;
		for (int32 i = 0; i < Boxes.Num(); ++i)
		{
			if (Boxes[i].Colors.Num() > 1 && Boxes[i].Range > BestRange)
			{
				BestRange = Boxes[i].Range;
				BestIdx = i;
			}
		}

		if (BestIdx < 0)
		{
			break; // Can't split any further
		}

		FColorBox& BoxToSplit = Boxes[BestIdx];
		int32 SplitChannel = BoxToSplit.Channel;

		// Sort by the split channel
		BoxToSplit.Colors.Sort([SplitChannel](const FColor& A, const FColor& B)
		{
			switch (SplitChannel)
			{
			case 0: return A.R < B.R;
			case 1: return A.G < B.G;
			default: return A.B < B.B;
			}
		});

		// Split at median
		int32 Median = BoxToSplit.Colors.Num() / 2;

		FColorBox NewBox;
		NewBox.Colors.Append(BoxToSplit.Colors.GetData() + Median, BoxToSplit.Colors.Num() - Median);
		BoxToSplit.Colors.SetNum(Median);

		BoxToSplit.ComputeStats();
		NewBox.ComputeStats();

		Boxes.Add(MoveTemp(NewBox));
	}

	// Build palette from box averages
	Palette.SetNum(256);
	for (int32 i = 0; i < 256; ++i)
	{
		if (i < Boxes.Num())
		{
			Palette[i] = Boxes[i].Average;
		}
		else
		{
			Palette[i] = FColor::Black;
		}
	}
}

uint8 FGifEncoder::FindNearestColor(const FColor& Color) const
{
	int32 BestIdx = 0;
	int32 BestDist = MAX_int32;

	for (int32 i = 0; i < Palette.Num(); ++i)
	{
		int32 DR = (int32)Color.R - (int32)Palette[i].R;
		int32 DG = (int32)Color.G - (int32)Palette[i].G;
		int32 DB = (int32)Color.B - (int32)Palette[i].B;
		int32 Dist = DR * DR + DG * DG + DB * DB;
		if (Dist < BestDist)
		{
			BestDist = Dist;
			BestIdx = i;
			if (Dist == 0) break;
		}
	}

	return static_cast<uint8>(BestIdx);
}

// ─────────────────────────────────────────────
// LZW compression
// ─────────────────────────────────────────────

void FGifEncoder::CompressLZW(TArray<uint8>& OutBytes, const TArray<uint8>& Indices, int32 MinCodeSize)
{
	const int32 ClearCode = 1 << MinCodeSize;
	const int32 EoiCode = ClearCode + 1;

	// Bit packing state
	int32 BitBuffer = 0;
	int32 BitsInBuffer = 0;
	TArray<uint8> RawBytes;

	auto EmitCode = [&](int32 Code, int32 CodeSize)
	{
		BitBuffer |= (Code << BitsInBuffer);
		BitsInBuffer += CodeSize;
		while (BitsInBuffer >= 8)
		{
			RawBytes.Add(static_cast<uint8>(BitBuffer & 0xFF));
			BitBuffer >>= 8;
			BitsInBuffer -= 8;
		}
	};

	// String table: maps (prefix_code, suffix_byte) -> code
	// Use a flat hash map for speed
	TMap<int64, int32> Table;
	int32 NextCode;
	int32 CodeSize;

	auto ResetTable = [&]()
	{
		Table.Reset();
		for (int32 i = 0; i < ClearCode; ++i)
		{
			Table.Add(static_cast<int64>(i), i);
		}
		NextCode = EoiCode + 1;
		CodeSize = MinCodeSize + 1;
	};

	ResetTable();
	EmitCode(ClearCode, CodeSize);

	if (Indices.Num() == 0)
	{
		EmitCode(EoiCode, CodeSize);
		if (BitsInBuffer > 0)
		{
			RawBytes.Add(static_cast<uint8>(BitBuffer & 0xFF));
		}
		// Write sub-blocks
		for (int32 Offset = 0; Offset < RawBytes.Num(); Offset += 255)
		{
			int32 BlockSize = FMath::Min(255, RawBytes.Num() - Offset);
			OutBytes.Add(static_cast<uint8>(BlockSize));
			OutBytes.Append(RawBytes.GetData() + Offset, BlockSize);
		}
		OutBytes.Add(0); // Block terminator
		return;
	}

	int32 CurrentPrefix = Indices[0];

	for (int32 i = 1; i < Indices.Num(); ++i)
	{
		int32 Suffix = Indices[i];
		int64 Key = (static_cast<int64>(CurrentPrefix) << 16) | Suffix;

		const int32* Found = Table.Find(Key);
		if (Found)
		{
			CurrentPrefix = *Found;
		}
		else
		{
			EmitCode(CurrentPrefix, CodeSize);

			if (NextCode < 4096)
			{
				Table.Add(Key, NextCode);
				NextCode++;
				if (NextCode == (1 << CodeSize) && CodeSize < 12)
				{
					CodeSize++;
				}
			}
			else
			{
				// Table full — emit clear code and reset
				EmitCode(ClearCode, CodeSize);
				ResetTable();
			}

			CurrentPrefix = Suffix;
		}
	}

	// Output remaining
	EmitCode(CurrentPrefix, CodeSize);
	EmitCode(EoiCode, CodeSize);

	// Flush remaining bits
	if (BitsInBuffer > 0)
	{
		RawBytes.Add(static_cast<uint8>(BitBuffer & 0xFF));
	}

	// Write sub-blocks (max 255 bytes each)
	for (int32 Offset = 0; Offset < RawBytes.Num(); Offset += 255)
	{
		int32 BlockSize = FMath::Min(255, RawBytes.Num() - Offset);
		OutBytes.Add(static_cast<uint8>(BlockSize));
		OutBytes.Append(RawBytes.GetData() + Offset, BlockSize);
	}
	OutBytes.Add(0); // Block terminator
}

// ─────────────────────────────────────────────
// GIF structure
// ─────────────────────────────────────────────

void FGifEncoder::WriteUint16(TArray<uint8>& Out, uint16 Value)
{
	Out.Add(static_cast<uint8>(Value & 0xFF));
	Out.Add(static_cast<uint8>((Value >> 8) & 0xFF));
}

void FGifEncoder::WriteHeader(TArray<uint8>& Out)
{
	// "GIF89a"
	const uint8 Header[] = { 'G', 'I', 'F', '8', '9', 'a' };
	Out.Append(Header, 6);
}

void FGifEncoder::WriteLogicalScreenDescriptor(TArray<uint8>& Out)
{
	WriteUint16(Out, static_cast<uint16>(Width));
	WriteUint16(Out, static_cast<uint16>(Height));

	// Packed field: Global Color Table Flag=1, Color Resolution=7 (8 bits), Sort=0, Size of GCT=7 (2^(7+1)=256)
	Out.Add(0xF7);
	Out.Add(0);   // Background color index
	Out.Add(0);   // Pixel aspect ratio
}

void FGifEncoder::WriteGlobalColorTable(TArray<uint8>& Out)
{
	for (int32 i = 0; i < 256; ++i)
	{
		Out.Add(Palette[i].R);
		Out.Add(Palette[i].G);
		Out.Add(Palette[i].B);
	}
}

void FGifEncoder::WriteNetscapeExtension(TArray<uint8>& Out)
{
	// Application Extension for infinite looping
	Out.Add(0x21); // Extension introducer
	Out.Add(0xFF); // Application extension label
	Out.Add(0x0B); // Block size = 11
	const uint8 Netscape[] = { 'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0' };
	Out.Append(Netscape, 11);
	Out.Add(0x03); // Sub-block size
	Out.Add(0x01); // Sub-block ID
	WriteUint16(Out, 0); // Loop count (0 = infinite)
	Out.Add(0x00); // Block terminator
}

void FGifEncoder::WriteGraphicsControlExtension(TArray<uint8>& Out, int32 DelayCs)
{
	Out.Add(0x21); // Extension introducer
	Out.Add(0xF9); // Graphic Control label
	Out.Add(0x04); // Block size
	Out.Add(0x00); // Packed: disposal=none, no user input, no transparent
	WriteUint16(Out, static_cast<uint16>(DelayCs));
	Out.Add(0x00); // Transparent color index (unused)
	Out.Add(0x00); // Block terminator
}

void FGifEncoder::WriteImageDescriptor(TArray<uint8>& Out)
{
	Out.Add(0x2C); // Image separator
	WriteUint16(Out, 0);  // Left
	WriteUint16(Out, 0);  // Top
	WriteUint16(Out, static_cast<uint16>(Width));
	WriteUint16(Out, static_cast<uint16>(Height));
	Out.Add(0x00); // Packed: no local color table, not interlaced
}

void FGifEncoder::WriteImageData(TArray<uint8>& Out, const TArray<uint8>& Indices)
{
	const int32 MinCodeSize = 8; // 256 colors
	Out.Add(static_cast<uint8>(MinCodeSize));
	CompressLZW(Out, Indices, MinCodeSize);
}

void FGifEncoder::WriteTrailer(TArray<uint8>& Out)
{
	Out.Add(0x3B);
}

// ─────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────

bool FGifEncoder::WriteToFile(const FString& FilePath)
{
	if (Frames.Num() == 0)
	{
		return false;
	}

	// Build global palette from all frames
	BuildGlobalPalette();

	TArray<uint8> FileData;
	FileData.Reserve(Width * Height * Frames.Num()); // rough estimate

	WriteHeader(FileData);
	WriteLogicalScreenDescriptor(FileData);
	WriteGlobalColorTable(FileData);
	WriteNetscapeExtension(FileData);

	// Encode each frame
	for (int32 FrameIdx = 0; FrameIdx < Frames.Num(); ++FrameIdx)
	{
		const TArray<FColor>& FramePixels = Frames[FrameIdx];

		// Quantize to palette indices
		TArray<uint8> Indices;
		Indices.SetNumUninitialized(Width * Height);
		for (int32 i = 0; i < FramePixels.Num(); ++i)
		{
			Indices[i] = FindNearestColor(FramePixels[i]);
		}

		WriteGraphicsControlExtension(FileData, FrameDelay);
		WriteImageDescriptor(FileData);
		WriteImageData(FileData, Indices);
	}

	WriteTrailer(FileData);

	return FFileHelper::SaveArrayToFile(FileData, *FilePath);
}
