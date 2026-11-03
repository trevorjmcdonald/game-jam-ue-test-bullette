#include "Verify/JamKitVerify.h"

#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "JamKit.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Verify/JamKitTrace.h"

namespace JamKitVerifyImpl
{
	/** Luma (0-255, as displayed) at or under which a pixel counts as near black, and at or over which as blown out. */
	constexpr int32 DarkLuma = 16;
	constexpr int32 BlownLuma = 250;

	int32 Percentile(const TArray<int64>& Histogram, int64 Total, double Fraction)
	{
		const int64 Target = static_cast<int64>(Total * Fraction);
		int64 Running = 0;
		for (int32 Bin = 0; Bin < Histogram.Num(); ++Bin)
		{
			Running += Histogram[Bin];
			if (Running > Target)
			{
				return Bin;
			}
		}
		return Histogram.Num() - 1;
	}
}

const FString& JamKitVerify::Args()
{
	static const FString Combined = []
	{
		const FString Path = FPaths::ProjectSavedDir() / TEXT("jamkit-args.txt");
		FString Marker;
		if (FFileHelper::LoadFileToString(Marker, *Path))
		{
			IFileManager::Get().Delete(*Path);
			UE_LOG(LogJamKit, Display, TEXT("Verify: arguments from %s: %s"), *Path, *Marker.TrimStartAndEnd());
		}
		Marker.ReplaceInline(TEXT("\r"), TEXT(" "));
		Marker.ReplaceInline(TEXT("\n"), TEXT(" "));
		return FString(FCommandLine::Get()) + TEXT(" ") + Marker.TrimStartAndEnd();
	}();
	return Combined;
}

bool JamKitVerify::HasParam(const TCHAR* Name)
{
	return FParse::Param(*Args(), Name);
}

bool JamKitVerify::GetValue(const TCHAR* KeyWithEquals, FString& OutValue)
{
	return FParse::Value(*Args(), KeyWithEquals, OutValue);
}

bool JamKitVerify::IsUnattended()
{
	FString Value;
	return HasParam(TEXT("jamprobe")) || GetValue(TEXT("jamprobe="), Value) || GetValue(TEXT("jamscript="), Value);
}

void JamKitVerify::AnalyseImage(TArray<FColor>& Pixels, FIntPoint Size, FString& OutJson, FString& OutLogLine)
{
	if (Pixels.IsEmpty() || Pixels.Num() != Size.X * Size.Y)
	{
		OutJson.Reset();
		OutLogLine = TEXT("(no picture)");
		return;
	}

	TArray<int64> Histogram;
	Histogram.SetNumZeroed(256);
	double Grid[3][3] = {};
	int64 GridCount[3][3] = {};
	double Sum = 0.0;
	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		const int32 Row = FMath::Min(Y * 3 / Size.Y, 2);
		for (int32 X = 0; X < Size.X; ++X)
		{
			FColor& Pixel = Pixels[Y * Size.X + X];
			Pixel.A = 255;
			const int32 Luma = FMath::Clamp(FMath::RoundToInt32(0.2126f * Pixel.R + 0.7152f * Pixel.G + 0.0722f * Pixel.B), 0, 255);
			++Histogram[Luma];
			Sum += Luma;
			const int32 Column = FMath::Min(X * 3 / Size.X, 2);
			Grid[Row][Column] += Luma;
			++GridCount[Row][Column];
		}
	}
	const int64 Total = Pixels.Num();
	int64 Dark = 0;
	int64 Blown = 0;
	for (int32 Bin = 0; Bin < 256; ++Bin)
	{
		Dark += Bin <= JamKitVerifyImpl::DarkLuma ? Histogram[Bin] : 0;
		Blown += Bin >= JamKitVerifyImpl::BlownLuma ? Histogram[Bin] : 0;
	}
	const double Mean = Sum / Total;
	const int32 P5 = JamKitVerifyImpl::Percentile(Histogram, Total, 0.05);
	const int32 P50 = JamKitVerifyImpl::Percentile(Histogram, Total, 0.5);
	const int32 P95 = JamKitVerifyImpl::Percentile(Histogram, Total, 0.95);

	FString GridJson = TEXT(",\"grid\":[");
	for (int32 Row = 0; Row < 3; ++Row)
	{
		for (int32 Column = 0; Column < 3; ++Column)
		{
			GridJson += FString::Printf(TEXT("%s%d"), Row || Column ? TEXT(",") : TEXT(""),
				FMath::RoundToInt32(Grid[Row][Column] / FMath::Max<int64>(GridCount[Row][Column], 1)));
		}
	}
	GridJson += TEXT("]");

	FJamKitTraceFields Stats;
	Stats.Add(TEXT("width"), Size.X).Add(TEXT("height"), Size.Y).Add(TEXT("mean"), Mean).Add(TEXT("p5"), P5).Add(TEXT("p50"), P50).Add(TEXT("p95"), P95)
		.Add(TEXT("darkPct"), 100.0 * Dark / Total).Add(TEXT("blownPct"), 100.0 * Blown / Total);
	OutJson = Stats.Json + GridJson;
	OutLogLine = FString::Printf(TEXT("mean %5.1f  p5 %3d  p50 %3d  p95 %3d  dark %4.1f%%  blown %4.1f%%"), Mean, P5, P50, P95,
		100.0 * Dark / Total, 100.0 * Blown / Total);
}

bool JamKitVerify::SavePng(const TArray<FColor>& Pixels, FIntPoint Size, const FString& Path)
{
	if (Pixels.IsEmpty() || Pixels.Num() != Size.X * Size.Y)
	{
		return false;
	}
	TArray64<uint8> Png;
	FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Png);
	if (!FFileHelper::SaveArrayToFile(Png, *Path))
	{
		UE_LOG(LogJamKit, Warning, TEXT("Verify: couldn't save %s"), *Path);
		return false;
	}
	return true;
}
