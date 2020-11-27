/* Copyright (C) Polypulse LLC - All Rights Reserved
 * Written by Sean Connor <sean@polypulse.io>, April 2020 */
#include "..\Public\LensCalibratorEditor.h"
#include "EditorSupportDelegates.h"
#include "IBlutilityModule.h"
#include "FileHelpers.h"

#define LOCTEXT_NAMESPACE "FLensCalibratorEditorModule"

void FLensCalibratorEditorModule::StartupModule()
{
	FLensCalibratorEditorCommands::Register();

	FEditorSupportDelegates::PrepareToCleanseEditorObject.AddRaw(this, &FLensCalibratorEditorModule::OnBeginTearDownLevel);
	createdUMGWidget = nullptr;

	const FLensCalibratorEditorCommands & commands = FLensCalibratorEditorCommands::Get();

	static TSharedRef<FLensCalibratorEditorModule> sp = MakeShareable(this);
	pluginCommands = MakeShareable(new FUICommandList);
	pluginCommands->MapAction(
		commands.openLensCalibrator,
		FExecuteAction::CreateSP(sp, &FLensCalibratorEditorModule::OpenLensCalibrator),
		FCanExecuteAction());

	pluginCommands->MapAction(
		commands.openExampleLevel,
		FExecuteAction::CreateSP(sp, &FLensCalibratorEditorModule::OpenExampleLevel),
		FCanExecuteAction());

	FLevelEditorModule& levelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<FExtensibilityManager> levelEditorMenuExtensibilityManager = levelEditorModule.GetMenuExtensibilityManager();

	TSharedPtr<FExtender> menuExtender = MakeShareable(new FExtender);
	menuExtender->AddMenuBarExtension(
		FName("Window"),
		EExtensionHook::After,
		pluginCommands,
		FMenuBarExtensionDelegate::CreateRaw(this, &FLensCalibratorEditorModule::MakeMenuEntry));

	levelEditorMenuExtensibilityManager->AddExtender(menuExtender);
}

void FLensCalibratorEditorModule::ShutdownModule()
{
    FLensCalibratorEditorCommands::Unregister();
}

void FLensCalibratorEditorModule::MakeMenuEntry(FMenuBarBuilder &menuBuilder)
{
	menuBuilder.AddPullDownMenu(
		FText::FromString("Polypulse"),
		FText::FromString("Lens Calibration Tools"),
		FNewMenuDelegate::CreateRaw(this, &FLensCalibratorEditorModule::MakeMenuSubMenu),
		"Polypulse",
		FName(TEXT("Polypulse"))
	);
}

void FLensCalibratorEditorModule::MakeMenuSubMenu(FMenuBuilder& menuBuilder)
{
	menuBuilder.AddMenuEntry(FLensCalibratorEditorCommands::Get().openLensCalibrator);
	menuBuilder.AddMenuEntry(FLensCalibratorEditorCommands::Get().openExampleLevel);
}

void FLensCalibratorEditorCommands::RegisterCommands()
{
	UI_COMMAND(openLensCalibrator, "Lens Calibrator", "Editor Widget for Lens Solver.", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(openExampleLevel, "Open Example Level", "Open the example level that demonstrates completed lens calibration.", EUserInterfaceActionType::Button, FInputGesture());
}

void FLensCalibratorEditorModule::OpenLensCalibrator()
{
	// UE_LOG(LogTemp, Warning, TEXT("It Works!!!"));
	FAssetRegistryModule* AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	FString path = "/LensCalibrator/Blueprints/LensSolver.LensSolver";
	FAssetData AssetData = AssetRegistryModule->Get().GetAssetByObjectPath(*path, false);
	if (!AssetData.IsValid())
	{
		UE_LOG(LogTemp, Fatal, TEXT("No editor utility widget blueprint at path: \"%s\"."), *path);
		return;
	}

	blueprint = static_cast<UEditorUtilityWidgetBlueprint*>(AssetData.GetAsset());

	FName RegistrationName = FName(*(blueprint->GetPathName() + TEXT("_ActiveTab")));
	FText DisplayName = FText::FromString(blueprint->GetName());
	FLevelEditorModule& levelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	TSharedPtr<FTabManager> levelEditorTabManager = levelEditorModule.GetLevelEditorTabManager();
	TSharedPtr<SDockTab> newDockTab = levelEditorTabManager->TryInvokeTab(RegistrationName);

	UWorld* world = GEditor->GetEditorWorldContext().World();

	if (world == NULL)
		return;

	levelEditorModule.OnMapChanged().AddRaw(this, &FLensCalibratorEditorModule::OnMapChanged);
	TSubclassOf<UEditorUtilityWidget> widgetClass = (UClass*)blueprint->GeneratedClass;
	createdUMGWidget = CreateWidget<UEditorUtilityWidget>(world, widgetClass);

	if (createdUMGWidget)
	{
		TSharedRef<SWidget> createdSlateWidget = createdUMGWidget->TakeWidget();
		newDockTab->SetContent(createdSlateWidget);
	}
}

void FLensCalibratorEditorModule::OnMapChanged(UWorld* InWorld, EMapChangeType MapChangeType)
{
	if (createdUMGWidget == nullptr)
		return;

	if (MapChangeType == EMapChangeType::TearDownWorld)
		createdUMGWidget->Rename(*createdUMGWidget->GetName(), GetTransientPackage());

	else if (MapChangeType == EMapChangeType::LoadMap || MapChangeType == EMapChangeType::NewMap)
	{
		UWorld* World = GEditor->GetEditorWorldContext().World();
		check(World);
		createdUMGWidget->Rename(*createdUMGWidget->GetName(), World);
	}
}

void FLensCalibratorEditorModule::OpenExampleLevel()
{
	FAssetRegistryModule* AssetRegistryModule = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	FString path = "/LensCalibrator/Test.Test";
	FAssetData AssetData = AssetRegistryModule->Get().GetAssetByObjectPath(*path, false);

	if (!AssetData.IsValid())
	{
		UE_LOG(LogTemp, Fatal, TEXT("No example map at path: \"%s\"."), *path);
		return;
	}

	if (!FEditorFileUtils::LoadMap(path, false, true))
	{
		UE_LOG(LogTemp, Fatal, TEXT("Unable to load example at path: \"%s\"."), *path);
		return;
	}
}

void FLensCalibratorEditorModule::OnBeginTearDownLevel(UObject * obj)
{
	/*
	if (createdUMGWidget == nullptr)
		return;

	createdUMGWidget->AddToRoot();
	createdUMGWidget->RemoveFromParent();
	createdUMGWidget->RemoveFromViewport();
	createdUMGWidget->ConditionalBeginDestroy();
	createdUMGWidget = nullptr;
	*/

	/*
	IBlutilityModule* BlutilityModule = FModuleManager::GetModulePtr<IBlutilityModule>("Blutility");
	if (BlutilityModule)
		BlutilityModule->RemoveLoadedScriptUI(blueprint);

	FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor"));
	if (LevelEditorModule)
	{
		TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule->GetLevelEditorTabManager();
		if (LevelEditorTabManager.IsValid())
			LevelEditorTabManager->UnregisterTabSpawner(blueprint->GetRegistrationName());
	}
	*/
}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FLensCalibratorEditorModule, LensCalibratorEditor)
