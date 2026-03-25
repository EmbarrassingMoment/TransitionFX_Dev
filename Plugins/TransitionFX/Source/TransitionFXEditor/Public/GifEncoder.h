// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"

/**
 * Minimal GIF89a encoder for capturing transition preview frames.
 * Supports multiple frames with a global 256-color palette,
 * LZW compression, and infinite looping.
 */
class FGifEncoder
{
public:
	/**
	 * @param InWidth       Image width in pixels.
	 * @param InHeight      Image height in pixels.
	 * @param InFrameDelay  Delay between frames in centiseconds (1/100 s). 3 ≈ 30fps.
	 */
	FGifEncoder(int32 InWidth, int32 InHeight, int32 InFrameDelay = 3);

	/** Add a frame. Pixels must be Width * Height elements (BGRA). */
	void AddFrame(const TArray<FColor>& Pixels);

	/** Encode all frames and write the GIF file. Returns true on success. */
	bool WriteToFile(const FString& FilePath);

private:
	// --- Color quantization (median-cut) ---
	/** Axis-aligned bounding box of colors for median-cut quantization. */
	struct FColorBox
	{
		TArray<FColor> Colors;
		int32 Channel; // 0=R, 1=G, 2=B — the axis to split on
		int32 Range;
		FColor Average;

		void ComputeStats();
	};

	void BuildGlobalPalette();
	uint8 FindNearestColor(const FColor& Color) const;

	// --- LZW compression ---
	void CompressLZW(TArray<uint8>& OutBytes, const TArray<uint8>& Indices, int32 MinCodeSize);

	// --- GIF structure writing ---
	void WriteHeader(TArray<uint8>& Out);
	void WriteLogicalScreenDescriptor(TArray<uint8>& Out);
	void WriteGlobalColorTable(TArray<uint8>& Out);
	void WriteNetscapeExtension(TArray<uint8>& Out);
	void WriteGraphicsControlExtension(TArray<uint8>& Out, int32 DelayCs);
	void WriteImageDescriptor(TArray<uint8>& Out);
	void WriteImageData(TArray<uint8>& Out, const TArray<uint8>& Indices);
	void WriteTrailer(TArray<uint8>& Out);

	// Helpers
	void WriteUint16(TArray<uint8>& Out, uint16 Value);

	int32 Width;
	int32 Height;
	int32 FrameDelay; // centiseconds

	TArray<FColor> Palette; // 256 entries
	TArray<TArray<FColor>> Frames;
};
