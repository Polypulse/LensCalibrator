#include "LensCalibratorEditor.h"
#include "LevelEditor.h"
#include "MultiBoxExtender.h"
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
	UE_LOG(LogTemp, Warning, TEXT("It Works!!!"));
}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FLensCalibratorEditorModule, LensCalibratorEditor)
