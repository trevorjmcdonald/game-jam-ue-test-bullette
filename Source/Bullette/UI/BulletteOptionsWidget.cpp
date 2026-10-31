#include "UI/BulletteOptionsWidget.h"

#include "Audio/BulletteAudio.h"
#include "Bullette.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "HAL/PlatformTime.h"
#include "InputMappingContext.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#define LOCTEXT_NAMESPACE "BulletteOptions"

namespace
{
	FBulletteBindingRowSpec MakeRow(const FText& DisplayName, FName KeyboardMapping, FName GamepadMapping, TArray<FKey> GamepadChoices = {})
	{
		FBulletteBindingRowSpec Spec;
		Spec.DisplayName = DisplayName;
		Spec.KeyboardMapping = KeyboardMapping;
		Spec.GamepadMapping = GamepadMapping;
		Spec.GamepadChoices = MoveTemp(GamepadChoices);
		return Spec;
	}

	FText KeyLabel(const FKey& Key)
	{
		if (!Key.IsValid())
		{
			return LOCTEXT("Unbound", "-");
		}
		if (Key == EKeys::Gamepad_Left2D)
		{
			return LOCTEXT("LeftStick", "Left Stick");
		}
		if (Key == EKeys::Gamepad_Right2D)
		{
			return LOCTEXT("RightStick", "Right Stick");
		}
		if (Key == EKeys::Gamepad_Special_Right)
		{
			return LOCTEXT("Start", "Start");
		}
		if (Key == EKeys::Gamepad_Special_Left)
		{
			return LOCTEXT("Select", "Select");
		}
		return Key.GetDisplayName(false);
	}

	const FText& DefaultHint()
	{
		static const FText Hint = LOCTEXT("Hint", "Select a key to change it.");
		return Hint;
	}
}

UBulletteOptionsWidget::UBulletteOptionsWidget()
{
	// Names match the player-mappable key settings on IMC_Default's mappings.
	Bindings = {
		MakeRow(LOCTEXT("MoveUp", "Move Up"), TEXT("MoveUp"), NAME_None),
		MakeRow(LOCTEXT("MoveDown", "Move Down"), TEXT("MoveDown"), NAME_None),
		MakeRow(LOCTEXT("MoveLeft", "Move Left"), TEXT("MoveLeft"), NAME_None),
		MakeRow(LOCTEXT("MoveRight", "Move Right"), TEXT("MoveRight"), NAME_None),
		MakeRow(LOCTEXT("MoveStick", "Move (Stick)"), NAME_None, TEXT("MoveStick"), { EKeys::Gamepad_Left2D, EKeys::Gamepad_Right2D }),
		MakeRow(LOCTEXT("Pause", "Pause"), TEXT("Pause"), TEXT("PauseGamepad")),
	};
}

UWidget* UBulletteOptionsWidget::GetDefaultFocusWidget() const
{
	return MasterSlider;
}

void UBulletteOptionsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	MasterSlider->OnValueChanged.AddDynamic(this, &UBulletteOptionsWidget::HandleMasterChanged);
	MusicSlider->OnValueChanged.AddDynamic(this, &UBulletteOptionsWidget::HandleMusicChanged);
	EffectsSlider->OnValueChanged.AddDynamic(this, &UBulletteOptionsWidget::HandleEffectsChanged);
	ResetButton->OnClicked.AddDynamic(this, &UBulletteOptionsWidget::HandleResetClicked);
	BackButton->OnClicked.AddDynamic(this, &UBulletteOptionsWidget::HandleBackClicked);

	// The main menu never applies IMC_Default, so register it here to have its mappings to list.
	UEnhancedInputUserSettings* InputSettings = GetInputSettings();
	if (InputSettings && BindingContext)
	{
		InputSettings->RegisterInputMappingContext(BindingContext);
	}
	else
	{
		UE_LOG(LogBullette, Warning, TEXT("%s: no Enhanced Input user settings (bEnableUserSettings) or BindingContext; rebinding is unavailable"), *GetName());
	}

	if (!ensureMsgf(RowClass, TEXT("%s has no RowClass"), *GetName()))
	{
		return;
	}
	for (const FBulletteBindingRowSpec& Spec : Bindings)
	{
		UBulletteKeyBindingRowWidget* Row = CreateWidget<UBulletteKeyBindingRowWidget>(GetOwningPlayer(), RowClass);
		Row->Setup(Spec);
		Row->OnRebindRequested.BindUObject(this, &UBulletteOptionsWidget::HandleRebindRequested);
		if (UVerticalBoxSlot* RowSlot = BindingList->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 2.0f));
		}
		AddFocusTarget(Row->GetKeyboardButton());
		AddFocusTarget(Row->GetGamepadButton());
		Rows.Add(Row);
	}
}

void UBulletteOptionsWidget::RefreshFromSettings()
{
	EndCapture();
	if (const UBulletteSettingsSubsystem* Settings = UBulletteSettingsSubsystem::Get(this))
	{
		const TPair<USlider*, UTextBlock*> Widgets[] = { { MasterSlider, MasterValueText }, { MusicSlider, MusicValueText }, { EffectsSlider, EffectsValueText } };
		const EBulletteVolume Volumes[] = { EBulletteVolume::Master, EBulletteVolume::Music, EBulletteVolume::Effects };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const float Value = Settings->GetVolume(Volumes[Index]);
			Widgets[Index].Key->SetValue(Value);
			Widgets[Index].Value->SetText(FText::AsPercent(Value));
		}
	}
}

void UBulletteOptionsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (CaptureRow.IsValid() && FPlatformTime::Seconds() - CaptureStartTime > CaptureTimeout)
	{
		EndCapture();
	}
}

FReply UBulletteOptionsWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	UBulletteKeyBindingRowWidget* Row = CaptureRow.Get();
	if (!Row)
	{
		return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
	}

	// While waiting for a key, eat everything so navigation and the buttons don't react to it.
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape)
	{
		EndCapture();
		return FReply::Handled();
	}

	// Stick directions arrive as digital keys too; sticks are chosen by cycling, not by pressing.
	const FString KeyName = Key.GetFName().ToString();
	const bool bStickDirection = KeyName.StartsWith(TEXT("Gamepad_LeftStick_")) || KeyName.StartsWith(TEXT("Gamepad_RightStick_"));
	if (Key.IsGamepadKey() == bCaptureGamepad && !Key.IsAnalog() && !bStickDirection && !Key.IsMouseButton() && !Key.IsTouch())
	{
		const FName Mapping = bCaptureGamepad ? Row->GetSpec().GamepadMapping : Row->GetSpec().KeyboardMapping;
		BindKey(Mapping, Key, bCaptureGamepad);
		EndCapture();
	}
	return FReply::Handled();
}

FReply UBulletteOptionsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (BackKeys.Contains(InKeyEvent.GetKey()))
	{
		HandleBackClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UBulletteOptionsWidget::HandleMasterChanged(float Value)
{
	SetVolume(EBulletteVolume::Master, Value, MasterValueText, true);
}

void UBulletteOptionsWidget::HandleMusicChanged(float Value)
{
	SetVolume(EBulletteVolume::Music, Value, MusicValueText, false);
}

void UBulletteOptionsWidget::HandleEffectsChanged(float Value)
{
	SetVolume(EBulletteVolume::Effects, Value, EffectsValueText, true);
}

void UBulletteOptionsWidget::SetVolume(EBulletteVolume Volume, float Value, UTextBlock* ValueText, bool bPreview)
{
	if (UBulletteSettingsSubsystem* Settings = UBulletteSettingsSubsystem::Get(this))
	{
		Settings->SetVolume(Volume, Value);
	}
	ValueText->SetText(FText::AsPercent(Value));

	// A short tick (an Effects sound) so the new effects level can be heard while dragging.
	const double Now = FPlatformTime::Seconds();
	if (bPreview && Now - LastPreviewTime > 0.15)
	{
		LastPreviewTime = Now;
		BulletteAudio::PlaySound(this, FocusSound, true, 0.0f);
	}
}

void UBulletteOptionsWidget::HandleRebindRequested(UBulletteKeyBindingRowWidget* Row, bool bGamepad)
{
	const FBulletteBindingRowSpec& Spec = Row->GetSpec();
	if (bGamepad && Spec.GamepadChoices.Num() > 0)
	{
		const int32 Current = Spec.GamepadChoices.IndexOfByKey(GetBoundKey(Spec.GamepadMapping));
		BindKey(Spec.GamepadMapping, Spec.GamepadChoices[(Current + 1) % Spec.GamepadChoices.Num()], true);
		return;
	}

	EndCapture();
	CaptureRow = Row;
	bCaptureGamepad = bGamepad;
	CaptureStartTime = FPlatformTime::Seconds();
	Row->ShowWaiting(bGamepad);
	HintText->SetText(FText::Format(bGamepad
		? LOCTEXT("CaptureGamepad", "Press a gamepad button for {0}. Esc cancels.")
		: LOCTEXT("CaptureKey", "Press a key for {0}. Esc cancels."), Spec.DisplayName));
}

UEnhancedInputUserSettings* UBulletteOptionsWidget::GetInputSettings() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	return Subsystem ? Subsystem->GetUserSettings() : nullptr;
}

FKey UBulletteOptionsWidget::GetBoundKey(FName MappingName) const
{
	const UEnhancedInputUserSettings* InputSettings = GetInputSettings();
	const FPlayerKeyMapping* Mapping = InputSettings && !MappingName.IsNone()
		? InputSettings->FindCurrentMappingForSlot(MappingName, EPlayerMappableKeySlot::First) : nullptr;
	return Mapping ? Mapping->GetCurrentKey() : EKeys::Invalid;
}

void UBulletteOptionsWidget::BindKey(FName MappingName, const FKey& Key, bool bGamepad)
{
	UEnhancedInputUserSettings* InputSettings = GetInputSettings();
	if (!InputSettings || MappingName.IsNone())
	{
		return;
	}

	auto Map = [InputSettings](FName Name, const FKey& NewKey)
	{
		FMapPlayerKeyArgs Args;
		Args.MappingName = Name;
		Args.Slot = EPlayerMappableKeySlot::First;
		Args.NewKey = NewKey;
		FGameplayTagContainer FailureReason;
		InputSettings->MapPlayerKey(Args, FailureReason);
		if (!FailureReason.IsEmpty())
		{
			UE_LOG(LogBullette, Warning, TEXT("Rebinding %s to %s failed: %s"), *Name.ToString(), *NewKey.ToString(), *FailureReason.ToStringSimple());
		}
	};

	const FKey OldKey = GetBoundKey(MappingName);
	for (const UBulletteKeyBindingRowWidget* Row : Rows)
	{
		const FName Other = bGamepad ? Row->GetSpec().GamepadMapping : Row->GetSpec().KeyboardMapping;
		if (!Other.IsNone() && Other != MappingName && GetBoundKey(Other) == Key)
		{
			Map(Other, OldKey);
		}
	}
	Map(MappingName, Key);

	InputSettings->ApplySettings();
	InputSettings->SaveSettings();
	UE_LOG(LogBullette, Display, TEXT("KeyBound mapping=%s key=%s"), *MappingName.ToString(), *Key.ToString());
	RefreshRows();
}

void UBulletteOptionsWidget::RefreshRows()
{
	for (UBulletteKeyBindingRowWidget* Row : Rows)
	{
		const FBulletteBindingRowSpec& Spec = Row->GetSpec();
		Row->ShowKeys(
			Spec.KeyboardMapping.IsNone() ? FText::GetEmpty() : KeyLabel(GetBoundKey(Spec.KeyboardMapping)),
			Spec.GamepadMapping.IsNone() ? FText::GetEmpty() : KeyLabel(GetBoundKey(Spec.GamepadMapping)));
	}
}

void UBulletteOptionsWidget::EndCapture()
{
	CaptureRow.Reset();
	HintText->SetText(DefaultHint());
	RefreshRows();
}

void UBulletteOptionsWidget::HandleResetClicked()
{
	if (UEnhancedInputUserSettings* InputSettings = GetInputSettings())
	{
		FGameplayTagContainer FailureReason;
		InputSettings->ResetKeyProfileIdToDefault(InputSettings->GetActiveKeyProfileId(), FailureReason);
		InputSettings->ApplySettings();
		InputSettings->SaveSettings();
	}
	EndCapture();
}

void UBulletteOptionsWidget::HandleBackClicked()
{
	EndCapture();
	if (UBulletteSettingsSubsystem* Settings = UBulletteSettingsSubsystem::Get(this))
	{
		Settings->SaveSettings();
	}
	OnClosed.ExecuteIfBound();
}

#undef LOCTEXT_NAMESPACE
