#include "LensCalibratorEditor.h"
#include "LevelEditor.h"
#include "MultiBoxExtender.h"
#include "AssetRegistryModule.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "EditorUtilityWidget.h"
#include "UObject/Class.h"
#include "..\Public\LensCalibratorEditor.h"

#define LOCTEXT_NAMESPACE "FLensCalibratorEditorModule"

void FLensCalibratorEditorModule::StartupModule()
{
	FLensCalibratorEditorCommands::Register();

	const FLensCalibratorEditorCommands & commands = FLensCalibratorEditorCommands::Get();

	static TSharedRef<FLensCalibratorEditorModule> sp = MakeShareable(this);
	pluginCommands = MakeShareable(new FUICommandList);
	pluginCommands->MapAction(
		commands.openLensCalibrator,
		FExecuteAction::CreateSP(sp, &FLensCalibratorEditorModule::OpenLensCalibrator),
		FCanExecuteAction());

	FLevelEditorModule& levelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	static TSharedPtr<FExtensibilityManager> levelEditorMenuExtensibilityManager = levelEditorModule.GetMenuExtensibilityManager();

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
	TSharedPtr<FUICommandInfo> openLensCalibrator = FLensCalibratorEditorCommands::Get().openLensCalibrator;
	menuBuilder.AddMenuEntry(openLensCalibrator);
}

void FLensCalibratorEditorCommands::RegisterCommands()
{
	UI_COMMAND(openLensCalibrator, "Lens Calibrator", "Editor Widget for Lens Solver.", EUserInterfaceActionType::Button, FInputGesture());
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

	UEditorUtilityWidgetBlueprint * blueprint = static_cast<UEditorUtilityWidgetBlueprint*>(AssetData.GetAsset());

	FName RegistrationName = FName(*(blueprint->GetPathName() + TEXT("_ActiveTab")));
	FText DisplayName = FText::FromString(blueprint->GetName());
	FLevelEditorModule& levelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	TSharedPtr<FTabManager> levelEditorTabManager = levelEditorModule.GetLevelEditorTabManager();
	TSharedRef<SDockTab> newDockTab = levelEditorTabManager->InvokeTab(RegistrationName);

	UWorld* world = GEditor->GetEditorWorldContext().World();
	check(world);

	TSubclassOf<UEditorUtilityWidget> widgetClass = (UClass*)blueprint->GeneratedClass;
	UEditorUtilityWidget* createdUMGWidget = CreateWidget<UEditorUtilityWidget>(world, widgetClass);

	if (createdUMGWidget)
	{
		TSharedRef<SWidget> createdSlateWidget = createdUMGWidget->TakeWidget();
		newDockTab->SetContent(createdSlateWidget);
	}
}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FLensCalibratorEditorModule, LensCalibratorEditor)
