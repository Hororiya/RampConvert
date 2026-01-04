// Copyright Epic Games, Inc. All Rights Reserved.

#include "RampConvert.h"
#include "Curves/CurveLinearColor.h"
#include "ContentBrowserModule.h"
#include "Engine/Texture2D.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "FRampConvertModule"

void FRampConvertModule::StartupModule()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	ContentBrowserMenuExtenderDelegate = FContentBrowserMenuExtender_SelectedAssets::CreateRaw(this, &FRampConvertModule::OnExtendContentBrowserTextureSelectionMenu);
	ContentBrowserModule.GetAllAssetViewContextMenuExtenders().Add(ContentBrowserMenuExtenderDelegate);
}

void FRampConvertModule::ShutdownModule()
{
	if (FContentBrowserModule* ContentBrowserModule = FModuleManager::GetModulePtr<FContentBrowserModule>(TEXT("ContentBrowser")))
	{
		TArray<FContentBrowserMenuExtender_SelectedAssets>& MenuExtenderDelegates = ContentBrowserModule->GetAllAssetViewContextMenuExtenders();
		MenuExtenderDelegates.RemoveAll([&](const FContentBrowserMenuExtender_SelectedAssets& Delegate){
			return Delegate.GetHandle() == ContentBrowserMenuExtenderDelegate.GetHandle();
		});
	}
}

TSharedRef<FExtender> FRampConvertModule::OnExtendContentBrowserTextureSelectionMenu(const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();
	if (SelectedAssets.Num() == 0)
	{
		return Extender;
	}
	
	bool bAllAreTextures = true;
	for (const FAssetData& Asset : SelectedAssets)
	{
		if (Asset.GetClass() != UTexture2D::StaticClass())
		{
			bAllAreTextures = false;
			break;
		}
	}

	if (bAllAreTextures)
	{
		Extender->AddMenuExtension(
			"GetAssetActions",
			EExtensionHook::After,
			nullptr,
			FMenuExtensionDelegate::CreateRaw(this, &FRampConvertModule::AddMenuEntry, SelectedAssets)
		);
	}

	return Extender;
}

void FRampConvertModule::AddMenuEntry(FMenuBuilder& MenuBuilder, TArray<FAssetData> SelectedAssets)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("GenerateRamp", "Generate Curve from Ramp Texture"),
		LOCTEXT("GenerateRampTooltip", "Generates curves from a ramp texture"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FRampConvertModule::OnGenerateRamps, SelectedAssets)
		)
	);
}

void FRampConvertModule::OnGenerateRamps(TArray<FAssetData> SelectedAssets)
{
	for (const FAssetData& Asset : SelectedAssets)
	{
		OnGenerateRamp(Asset);
	}
}

void FRampConvertModule::OnGenerateRamp(FAssetData SelectedAsset)
{
	UTexture2D* Texture = Cast<UTexture2D>(SelectedAsset.GetAsset());
	if (!Texture)
	{
		return;
	}
	
	FTextureSource& TextureSource = Texture->Source;
	ETextureSourceFormat SourceFormat = TextureSource.GetFormat();
	if (SourceFormat != TSF_BGRA8)
	{
		return;
	}
	int32 Width = Texture->GetSizeX();
	int32 Height = Texture->GetSizeY();
	
	TArray<uint8> RawData;
	uint8* MipData = (uint8*)TextureSource.LockMip(0);
	if (MipData && Width > 0 && Height > 0)
	{
		RawData.SetNumUninitialized(Width * Height * 4);
		FMemory::Memcpy(RawData.GetData(),MipData,RawData.Num());
	}
	TextureSource.UnlockMip(0);
	if (RawData.Num() == 0)
	{
		return;
	}
	
	FString BasePackageName = SelectedAsset.PackageName.ToString();
	FString BaseAssetName = SelectedAsset.AssetName.ToString();
	
	for (int32 y = 0; y < Height; ++y)
	{
		FString Suffix = FString::Printf(TEXT("_Curve_%d"),y);
		FString PackageName = BasePackageName + Suffix;
		FString AssetName = BaseAssetName + Suffix;

		UPackage* Package = CreatePackage(*PackageName);
		Package->FullyLoad();

		UCurveLinearColor* Curve = NewObject<UCurveLinearColor>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (Curve)
		{
			for (int x = 0; x < Width; ++x)
			{
				int32 Index = (y * Width + x) * 4;
				uint8 R, G, B, A;
				
				if (SourceFormat == TSF_BGRA8)
				{
					B = RawData[Index];
					G = RawData[Index + 1];
					R = RawData[Index + 2];
					A = RawData[Index + 3];
				}
				else
				{
					R = RawData[Index];
					G = RawData[Index + 1];
					B = RawData[Index + 2];
					A = RawData[Index + 3];
				}
				
				FLinearColor LinearColor;
				if (Texture->SRGB)
				{
					LinearColor = FLinearColor(FColor(R, G, B, A));
				}
				else
				{
					LinearColor = FLinearColor(R / 255.f, G / 255.f, B / 255.f, A / 255.f);
				}
				
				float Time = (Width > 1) ? (float)x / (float)(Width - 1) : 0.f;
				
				Curve->FloatCurves[0].AddKey(Time, LinearColor.R);
				Curve->FloatCurves[1].AddKey(Time, LinearColor.G);
				Curve->FloatCurves[2].AddKey(Time, LinearColor.B);
				Curve->FloatCurves[3].AddKey(Time, LinearColor.A);
			}
			FAssetRegistryModule::AssetCreated(Curve);
			Curve->MarkPackageDirty();
		}
	}
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRampConvertModule, RampConvert)