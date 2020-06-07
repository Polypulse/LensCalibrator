#include "LensSolverFactory.h"

UObject* ULensSolverFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	/*
	auto NewMediaPlayer = NewObject<UMediaPlayer>(InParent, InClass, InName, Flags);

	if ((NewMediaPlayer != nullptr) && Options.CreateVideoTexture)
	{
		IAssetTools& AssetTools = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		const FString ParentName = InParent->GetOutermost()->GetName();

		FString OutAssetName;
		FString OutPackageName;

		AssetTools.CreateUniqueAssetName(ParentName, TEXT("_Video"), OutPackageName, OutAssetName);
		const FString PackagePath = FPackageName::GetLongPackagePath(OutPackageName);
		auto Factory = NewObject<UMediaTextureFactoryNew>();
		auto VideoTexture = Cast<UMediaTexture>(AssetTools.CreateAsset(OutAssetName, PackagePath, UMediaTexture::StaticClass(), Factory));

		if (VideoTexture != nullptr)
		{
			VideoTexture->SetDefaultMediaPlayer(NewMediaPlayer);
		}
	}

	return NewMediaPlayer;
	*/
}