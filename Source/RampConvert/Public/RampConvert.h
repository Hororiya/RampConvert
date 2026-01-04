// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "ContentBrowserDelegates.h"
#include "AssetRegistry/AssetData.h"

class FRampConvertModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
private:
	TSharedRef<FExtender> OnExtendContentBrowserTextureSelectionMenu(const TArray<FAssetData>& SelectedAssets);
	void AddMenuEntry(FMenuBuilder& MenuBuilder, TArray<FAssetData> SelectedAssets);
	void OnGenerateRamps(TArray<FAssetData> SelectedAssets);
	void OnGenerateRamp(FAssetData SelectedAsset);

	FContentBrowserMenuExtender_SelectedAssets ContentBrowserMenuExtenderDelegate;
};
