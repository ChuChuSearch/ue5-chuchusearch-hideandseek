#include "PropIconCommandlet.h"

#include "PropBase.h"
#include "PropIconSet.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetCompilingManager.h"
#include "RenderingThread.h"
#include "Engine/Blueprint.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/ObjectThumbnail.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "ObjectTools.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#endif

UPropIconCommandlet::UPropIconCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

#if WITH_EDITOR
namespace
{
    const TCHAR* PropFolder = TEXT("/Game/Game/Blueprints/Props");
    const TCHAR* IconFolder = TEXT("/Game/Game/Textures/PropIcons");
    const TCHAR* IconSetPath = TEXT("/Game/Game/Data/DA_PropIcons");

    bool SaveAsset(UObject* Asset)
    {
        UPackage* Package = Asset->GetOutermost();
        Package->MarkPackageDirty();
        const FString FileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        return UPackage::SavePackage(Package, Asset, *FileName, Args);
    }

    // 테두리에서 시작해 배경색과 비슷한 픽셀을 투명하게 만든다(물체 안쪽 같은 색은 남는다)
    void MakeBackgroundTransparent(TArray<FColor>& Pixels, int32 W, int32 H)
    {
        if (W <= 0 || H <= 0)
        {
            return;
        }

        // 썸네일 배경은 회색 체크무늬(두 색)다. 테두리에서 가장 많이 나오는 색들을 배경색으로 본다.
        TMap<uint32, int32> BorderCounts;
        int32 BorderTotal = 0;
        auto CountBorder = [&](int32 X, int32 Y)
        {
            const FColor& C = Pixels[Y * W + X];
            BorderCounts.FindOrAdd(FColor(C.R, C.G, C.B, 255).ToPackedARGB())++;
            ++BorderTotal;
        };
        for (int32 X = 0; X < W; ++X) { CountBorder(X, 0); CountBorder(X, H - 1); }
        for (int32 Y = 0; Y < H; ++Y) { CountBorder(0, Y); CountBorder(W - 1, Y); }

        TArray<FColor> BgColors;
        BorderCounts.ValueSort([](int32 A, int32 B) { return A > B; });
        for (const TPair<uint32, int32>& Pair : BorderCounts)
        {
            if (Pair.Value * 20 < BorderTotal || BgColors.Num() >= 4)
            {
                break;
            }
            BgColors.Add(FColor(Pair.Key));
        }
        if (BgColors.Num() == 0)
        {
            BgColors.Add(Pixels[0]);
        }

        auto IsBg = [&BgColors](const FColor& C)
        {
            for (const FColor& Bg : BgColors)
            {
                if (FMath::Abs(C.R - Bg.R) <= 8 && FMath::Abs(C.G - Bg.G) <= 8 && FMath::Abs(C.B - Bg.B) <= 8)
                {
                    return true;
                }
            }
            return false;
        };

        TArray<bool> Visited;
        Visited.Init(false, W * H);
        TArray<int32> Stack;
        auto Push = [&](int32 X, int32 Y)
        {
            if (X < 0 || Y < 0 || X >= W || Y >= H)
            {
                return;
            }
            const int32 I = Y * W + X;
            if (!Visited[I] && IsBg(Pixels[I]))
            {
                Visited[I] = true;
                Stack.Add(I);
            }
        };

        for (int32 X = 0; X < W; ++X) { Push(X, 0); Push(X, H - 1); }
        for (int32 Y = 0; Y < H; ++Y) { Push(0, Y); Push(W - 1, Y); }

        while (Stack.Num() > 0)
        {
            const int32 I = Stack.Pop(EAllowShrinking::No);
            Pixels[I].A = 0;
            const int32 X = I % W;
            const int32 Y = I / W;
            Push(X + 1, Y);
            Push(X - 1, Y);
            Push(X, Y + 1);
            Push(X, Y - 1);
        }

        // 가장자리 한 줄은 살짝 투명하게(배경 쪽 계단 현상 줄이기)
        for (int32 Y = 1; Y < H - 1; ++Y)
        {
            for (int32 X = 1; X < W - 1; ++X)
            {
                const int32 I = Y * W + X;
                if (Pixels[I].A == 0)
                {
                    continue;
                }
                const bool bEdge = Pixels[I - 1].A == 0 || Pixels[I + 1].A == 0 || Pixels[I - W].A == 0 || Pixels[I + W].A == 0;
                if (bEdge)
                {
                    Pixels[I].A = 170;
                }
            }
        }
    }

    // 물체가 있는 부분만 잘라 정사각형 가운데에 크게 놓는다(썸네일은 물체가 작고 한쪽에 치우쳐 나온다)
    TArray<FColor> CropAndCenter(const TArray<FColor>& Pixels, int32 W, int32 H, int32 OutSize)
    {
        int32 MinX = W, MinY = H, MaxX = -1, MaxY = -1;
        for (int32 Y = 0; Y < H; ++Y)
        {
            for (int32 X = 0; X < W; ++X)
            {
                if (Pixels[Y * W + X].A > 0)
                {
                    MinX = FMath::Min(MinX, X);
                    MaxX = FMath::Max(MaxX, X);
                    MinY = FMath::Min(MinY, Y);
                    MaxY = FMath::Max(MaxY, Y);
                }
            }
        }

        TArray<FColor> Out;
        Out.Init(FColor(0, 0, 0, 0), OutSize * OutSize);
        if (MaxX < MinX || MaxY < MinY)
        {
            return Out;
        }

        // 물체 둘레에 8% 여백을 두고 정사각형으로
        const float BoxW = float(MaxX - MinX + 1);
        const float BoxH = float(MaxY - MinY + 1);
        const float Side = FMath::Max(BoxW, BoxH) * 1.16f;
        const float CenterX = (MinX + MaxX + 1) * 0.5f;
        const float CenterY = (MinY + MaxY + 1) * 0.5f;
        const float SrcX0 = CenterX - Side * 0.5f;
        const float SrcY0 = CenterY - Side * 0.5f;
        const float Step = Side / OutSize;

        auto Sample = [&](float SX, float SY) -> FLinearColor
        {
            const int32 IX = FMath::FloorToInt(SX);
            const int32 IY = FMath::FloorToInt(SY);
            if (IX < 0 || IY < 0 || IX >= W || IY >= H)
            {
                return FLinearColor(0.f, 0.f, 0.f, 0.f);
            }
            const FColor& C = Pixels[IY * W + IX];
            const float A = C.A / 255.f;
            // 투명한 배경 색이 섞여 테두리가 어두워지지 않게 미리 알파를 곱해 둔다
            return FLinearColor(C.R / 255.f * A, C.G / 255.f * A, C.B / 255.f * A, A);
        };

        // 축소/확대 모두 무난하도록 한 칸을 2x2로 나눠 평균
        for (int32 Y = 0; Y < OutSize; ++Y)
        {
            for (int32 X = 0; X < OutSize; ++X)
            {
                FLinearColor Sum(0.f, 0.f, 0.f, 0.f);
                for (int32 SY = 0; SY < 2; ++SY)
                {
                    for (int32 SX = 0; SX < 2; ++SX)
                    {
                        Sum += Sample(SrcX0 + (X + (SX + 0.5f) * 0.5f) * Step, SrcY0 + (Y + (SY + 0.5f) * 0.5f) * Step);
                    }
                }
                Sum *= 0.25f;
                if (Sum.A > KINDA_SMALL_NUMBER)
                {
                    Out[Y * OutSize + X] = FColor(
                        uint8(FMath::Clamp(Sum.R / Sum.A, 0.f, 1.f) * 255.f + 0.5f),
                        uint8(FMath::Clamp(Sum.G / Sum.A, 0.f, 1.f) * 255.f + 0.5f),
                        uint8(FMath::Clamp(Sum.B / Sum.A, 0.f, 1.f) * 255.f + 0.5f),
                        uint8(FMath::Clamp(Sum.A, 0.f, 1.f) * 255.f + 0.5f));
                }
            }
        }
        return Out;
    }

    UTexture2D* CreateIconTexture(const FString& AssetName, const TArray<FColor>& Pixels, int32 W, int32 H)
    {
        const FString PackageName = FString::Printf(TEXT("%s/%s"), IconFolder, *AssetName);
        UPackage* Package = CreatePackage(*PackageName);
        Package->FullyLoad();

        UTexture2D* Texture = FindObject<UTexture2D>(Package, *AssetName);
        const bool bNew = Texture == nullptr;
        if (bNew)
        {
            Texture = NewObject<UTexture2D>(Package, *AssetName, RF_Public | RF_Standalone);
        }

        Texture->PreEditChange(nullptr);
        Texture->Source.Init(W, H, 1, 1, TSF_BGRA8, reinterpret_cast<const uint8*>(Pixels.GetData()));
        Texture->CompressionSettings = TC_EditorIcon;
        Texture->LODGroup = TEXTUREGROUP_UI;
        Texture->MipGenSettings = TMGS_NoMipmaps;
        Texture->SRGB = true;
        Texture->NeverStream = true;
        Texture->PostEditChange();

        if (bNew)
        {
            FAssetRegistryModule::AssetCreated(Texture);
        }

        SaveAsset(Texture);
        return Texture;
    }
}
#endif

int32 UPropIconCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
    int32 Size = 256;
    FParse::Value(*Params, TEXT("Size="), Size);
    Size = FMath::Clamp(Size, 64, 1024);

    FString Only;
    FParse::Value(*Params, TEXT("Only="), Only);

    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);

    FARFilter Filter;
    Filter.PackagePaths.Add(FName(PropFolder));
    Filter.bRecursivePaths = true;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());

    TArray<FAssetData> Assets;
    Registry.GetAssets(Filter, Assets);
    Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.AssetName.LexicalLess(B.AssetName); });

    // 목록 에셋(있으면 이어서 채운다)
    const FString SetAssetName = FPackageName::GetShortName(IconSetPath);
    UPackage* SetPackage = CreatePackage(IconSetPath);
    SetPackage->FullyLoad();
    UPropIconSet* IconSet = FindObject<UPropIconSet>(SetPackage, *SetAssetName);
    const bool bNewSet = IconSet == nullptr;
    if (bNewSet)
    {
        IconSet = NewObject<UPropIconSet>(SetPackage, *SetAssetName, RF_Public | RF_Standalone);
    }

    const FString PreviewDir = FPaths::ProjectSavedDir() / TEXT("PropIcons");
    IFileManager::Get().MakeDirectory(*PreviewDir, true);

    int32 Made = 0;
    int32 Skipped = 0;
    for (const FAssetData& Asset : Assets)
    {
        const FString BPName = Asset.AssetName.ToString();
        if (!Only.IsEmpty() && BPName != Only)
        {
            continue;
        }

        UBlueprint* BP = Cast<UBlueprint>(Asset.GetAsset());
        UClass* PropClass = BP ? BP->GeneratedClass : nullptr;
        if (!PropClass || !PropClass->IsChildOf(APropBase::StaticClass()) || PropClass->HasAnyClassFlags(CLASS_Abstract))
        {
            ++Skipped;
            continue;
        }

        // 셰이더/텍스처 컴파일이 다 끝난 뒤에 찍어야 검게 나오지 않는다
        FAssetCompilingManager::Get().FinishAllCompilation();

        // 블루프린트 썸네일(액터와 모든 구성요소가 보이는 사진)을 찍는다.
        // 처음 한 번은 장면/텍스처 준비용으로 버리고 두 번째 것을 쓴다.
        FObjectThumbnail Thumb;
        for (int32 Pass = 0; Pass < 2; ++Pass)
        {
            Thumb = FObjectThumbnail();
            ThumbnailTools::RenderThumbnail(BP, Size * 2, Size * 2, ThumbnailTools::EThumbnailTextureFlushMode::AlwaysFlush, nullptr, &Thumb);
            FlushRenderingCommands();
            FAssetCompilingManager::Get().FinishAllCompilation();
        }

        const int32 W = Thumb.GetImageWidth();
        const int32 H = Thumb.GetImageHeight();
        const TArray<uint8>& Raw = Thumb.GetUncompressedImageData();
        if (W <= 0 || H <= 0 || Raw.Num() < W * H * 4)
        {
            UE_LOG(LogTemp, Warning, TEXT("PropIcon: thumbnail failed for %s"), *BPName);
            ++Skipped;
            continue;
        }

        TArray<FColor> Pixels;
        Pixels.SetNumUninitialized(W * H);
        FMemory::Memcpy(Pixels.GetData(), Raw.GetData(), W * H * 4);   // BGRA 순서 = FColor 메모리 순서
        for (FColor& C : Pixels)
        {
            C.A = 255;
        }

        if (FParse::Param(*Params, TEXT("SaveRaw")))
        {
            TArray64<uint8> RawPng;
            FImageUtils::PNGCompressImageArray(W, H, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), RawPng);
            FFileHelper::SaveArrayToFile(RawPng, *(PreviewDir / (BPName + TEXT("_raw.png"))));
        }

        MakeBackgroundTransparent(Pixels, W, H);

        // 크게 찍어서(Size의 2배) 물체만 잘라 Size로 줄이면 더 선명하다
        Pixels = CropAndCenter(Pixels, W, H, Size);
        const int32 OutW = Size;
        const int32 OutH = Size;

        UTexture2D* Icon = CreateIconTexture(FString::Printf(TEXT("T_PropIcon_%s"), *BPName), Pixels, OutW, OutH);
        if (!Icon)
        {
            ++Skipped;
            continue;
        }

        IconSet->Icons.Add(TSoftClassPtr<AActor>(PropClass), Icon);

        // 확인용 PNG
        TArray64<uint8> Png;
        FImageUtils::PNGCompressImageArray(OutW, OutH, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Png);
        FFileHelper::SaveArrayToFile(Png, *(PreviewDir / (BPName + TEXT(".png"))));

        ++Made;
        UE_LOG(LogTemp, Display, TEXT("PropIcon: %s (%dx%d)"), *BPName, W, H);
    }

    if (bNewSet)
    {
        FAssetRegistryModule::AssetCreated(IconSet);
    }
    SaveAsset(IconSet);

    UE_LOG(LogTemp, Display, TEXT("PropIcon: done. made=%d skipped=%d total=%d"), Made, Skipped, IconSet->Icons.Num());
    return 0;
#else
    return 1;
#endif
}
