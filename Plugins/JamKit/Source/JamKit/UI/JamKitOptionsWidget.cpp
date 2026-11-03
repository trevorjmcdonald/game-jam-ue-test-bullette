#include "UI/JamKitOptionsWidget.h"

#include "Audio/JamKitAudio.h"
#include "JamKit.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "HAL/PlatformTime.h"
#include "InputMappingContext.h"
#include "Settings/JamKitSettings.h"
#include "Settings/JamKitSettingsSubsystem.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#define LOCTEXT_NAMESPACE "JamKitOptions"

namespace
{
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

UJamKitOptionsWidget::UJamKitOptionsWidget()
{
}

TArray<FJamKitBindingRowSpec> UJamKitOptionsWidget::BuildRows(const UInputMappingContext* Context)
{
	TArray<FJamKitBindingRowSpec> Rows;
	if (!Context)
	{
		return Rows;
	}
	for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
	{
		if (!Mapping.IsPlayerMappable() || Mapping.GetMappingName().IsNone())
		{
			continue;
		}
		const FText& Name = Mapping.GetDisplayName().IsEmpty() ? FText::FromName(Mapping.GetMappingName()) : Mapping.GetDisplayName();
		FJamKitBindingRowSpec* Row = Rows.FindByPredicate([&Name](const FJamKitBindingRowSpec& Spec) { return Spec.DisplayName.EqualTo(Name); });
		if (!Row)
		{
			Row = &Rows.AddDefaulted_GetRef();
			Row->DisplayName = Name;
		}

		const FKey& Key = Mapping.Key;
		if (Key.IsGamepadKey())
		{
			Row->GamepadMapping = Mapping.GetMappingName();
			// Sticks can't be "pressed" to capture them; the button cycles between the two instead.
			if (Key == EKeys::Gamepad_Left2D || Key == EKeys::Gamepad_Right2D)
			{
				Row->GamepadChoices = { EKeys::Gamepad_Left2D, EKeys::Gamepad_Right2D };
			}
		}
		else
		{
			Row->KeyboardMapping = Mapping.GetMappingName();
		}
	}
	return Rows;
}

UWidget* UJamKitOptionsWidget::GetDefaultFocusWidget() const
{
	return MasterSlider;
}

void UJamKitOptionsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	MasterSlider->OnValueChanged.AddDynamic(this, &UJamKitOptionsWidget::HandleMasterChanged);
	MusicSlider->OnValueChanged.AddDynamic(this, &UJamKitOptionsWidget::HandleMusicChanged);
	EffectsSlider->OnValueChanged.AddDynamic(this, &UJamKitOptionsWidget::HandleEffectsChanged);
	ResetButton->OnClicked.AddDynamic(this, &UJamKitOptionsWidget::HandleResetClicked);
	BackButton->OnClicked.AddDynamic(this, &UJamKitOptionsWidget::HandleBackClicked);

	// The main menu never applies the gameplay context, so register it here to have its mappings to list.
	const UInputMappingContext* BindingContext = GetDefault<UJamKitSettings>()->RebindContext.LoadSynchronous();
	UEnhancedInputUserSettings* InputSettings = GetInputSettings();
	if (InputSettings && BindingContext)
	{
		InputSettings->RegisterInputMappingContext(BindingContext);
	}
	else
	{
		UE_LOG(LogJamKit, Warning, TEXT("%s: no Enhanced Input user settings (bEnableUserSettings) or RebindContext (Project Settings > Jam Kit); rebinding is unavailable"), *GetName());
	}

	if (!ensureMsgf(RowClass, TEXT("%s has no RowClass"), *GetName()))
	{
		return;
	}
	TArray<FJamKitBindingRowSpec> RowSpecs = Bindings.IsEmpty() ? BuildRows(BindingContext) : Bindings;
	if (bInvertLookRow)
	{
		FJamKitBindingRowSpec Invert;
		Invert.Kind = EJamKitOptionRow::InvertLook;
		Invert.DisplayName = LOCTEXT("InvertLook", "Invert look up/down");
		// Names only so both columns show; this row toggles settings instead of binding keys.
		Invert.KeyboardMapping = TEXT("InvertLookMouse");
		Invert.GamepadMapping = TEXT("InvertLookGamepad");
		const int32 LookIndex = RowSpecs.IndexOfByPredicate([this](const FJamKitBindingRowSpec& Spec) { return Spec.DisplayName.ToString().StartsWith(LookRowName.ToString()); });
		RowSpecs.Insert(Invert, LookIndex == INDEX_NONE ? 0 : LookIndex + 1);
	}
	for (const FJamKitBindingRowSpec& Spec : RowSpecs)
	{
		UJamKitKeyBindingRowWidget* Row = CreateWidget<UJamKitKeyBindingRowWidget>(GetOwningPlayer(), RowClass);
		Row->Setup(Spec);
		Row->OnRebindRequested.BindUObject(this, &UJamKitOptionsWidget::HandleRebindRequested);
		if (UVerticalBoxSlot* RowSlot = BindingList->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 2.0f));
		}
		AddFocusTarget(Row->GetKeyboardButton());
		AddFocusTarget(Row->GetGamepadButton());
		Rows.Add(Row);
	}
}

void UJamKitOptionsWidget::OnOpened()
{
	EndCapture();
	if (const UJamKitSettingsSubsystem* Settings = UJamKitSettingsSubsystem::Get(this))
	{
		const TPair<USlider*, UTextBlock*> Widgets[] = { { MasterSlider, MasterValueText }, { MusicSlider, MusicValueText }, { EffectsSlider, EffectsValueText } };
		const EJamKitVolume Volumes[] = { EJamKitVolume::Master, EJamKitVolume::Music, EJamKitVolume::Effects };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const float Value = Settings->GetVolume(Volumes[Index]);
			Widgets[Index].Key->SetValue(Value);
			Widgets[Index].Value->SetText(FText::AsPercent(Value));
		}
	}
}

void UJamKitOptionsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (CaptureRow.IsValid() && FPlatformTime::Seconds() - CaptureStartTime > CaptureTimeout)
	{
		EndCapture();
	}
}

FReply UJamKitOptionsWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	UJamKitKeyBindingRowWidget* Row = CaptureRow.Get();
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

void UJamKitOptionsWidget::HandleMasterChanged(float Value)
{
	SetVolume(EJamKitVolume::Master, Value, MasterValueText, true);
}

void UJamKitOptionsWidget::HandleMusicChanged(float Value)
{
	SetVolume(EJamKitVolume::Music, Value, MusicValueText, false);
}

void UJamKitOptionsWidget::HandleEffectsChanged(float Value)
{
	SetVolume(EJamKitVolume::Effects, Value, EffectsValueText, true);
}

void UJamKitOptionsWidget::SetVolume(EJamKitVolume Volume, float Value, UTextBlock* ValueText, bool bPreview)
{
	if (UJamKitSettingsSubsystem* Settings = UJamKitSettingsSubsystem::Get(this))
	{
		Settings->SetVolume(Volume, Value);
	}
	ValueText->SetText(FText::AsPercent(Value));

	// A short tick (an Effects sound) so the new effects level can be heard while dragging.
	const double Now = FPlatformTime::Seconds();
	if (bPreview && Now - LastPreviewTime > 0.15)
	{
		LastPreviewTime = Now;
		JamKitAudio::PlaySound(this, FocusSound, true, 0.0f);
	}
}

void UJamKitOptionsWidget::HandleRebindRequested(UJamKitKeyBindingRowWidget* Row, bool bGamepad)
{
	const FJamKitBindingRowSpec& Spec = Row->GetSpec();
	if (Spec.Kind == EJamKitOptionRow::InvertLook)
	{
		// A toggle, not a rebind. Saved with the volumes when the screen closes.
		if (UJamKitSettingsSubsystem* Settings = UJamKitSettingsSubsystem::Get(this))
		{
			Settings->SetInvertLook(bGamepad, !Settings->GetInvertLook(bGamepad));
		}
		EndCapture();
		return;
	}
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

UEnhancedInputUserSettings* UJamKitOptionsWidget::GetInputSettings() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	return Subsystem ? Subsystem->GetUserSettings() : nullptr;
}

FKey UJamKitOptionsWidget::GetBoundKey(FName MappingName) const
{
	const UEnhancedInputUserSettings* InputSettings = GetInputSettings();
	const FPlayerKeyMapping* Mapping = InputSettings && !MappingName.IsNone()
		? InputSettings->FindCurrentMappingForSlot(MappingName, EPlayerMappableKeySlot::First) : nullptr;
	return Mapping ? Mapping->GetCurrentKey() : EKeys::Invalid;
}

void UJamKitOptionsWidget::BindKey(FName MappingName, const FKey& Key, bool bGamepad)
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
			UE_LOG(LogJamKit, Warning, TEXT("Rebinding %s to %s failed: %s"), *Name.ToString(), *NewKey.ToString(), *FailureReason.ToStringSimple());
		}
	};

	const FKey OldKey = GetBoundKey(MappingName);
	for (const UJamKitKeyBindingRowWidget* Row : Rows)
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
	UE_LOG(LogJamKit, Display, TEXT("KeyBound mapping=%s key=%s"), *MappingName.ToString(), *Key.ToString());
	RefreshRows();
}

void UJamKitOptionsWidget::RefreshRows()
{
	const UJamKitSettingsSubsystem* Settings = UJamKitSettingsSubsystem::Get(this);
	for (UJamKitKeyBindingRowWidget* Row : Rows)
	{
		const FJamKitBindingRowSpec& Spec = Row->GetSpec();
		if (Spec.Kind == EJamKitOptionRow::InvertLook)
		{
			auto OnOff = [Settings](bool bGamepad) { return Settings && Settings->GetInvertLook(bGamepad) ? LOCTEXT("On", "On") : LOCTEXT("Off", "Off"); };
			Row->ShowKeys(FText::Format(LOCTEXT("MouseToggle", "Mouse: {0}"), OnOff(false)), FText::Format(LOCTEXT("PadToggle", "Controller: {0}"), OnOff(true)));
			continue;
		}
		const FKey GamepadKey = Spec.GamepadMapping.IsNone() ? FKey() : GetBoundKey(Spec.GamepadMapping);
		Row->ShowKeys(
			Spec.KeyboardMapping.IsNone() ? FText::GetEmpty() : KeyLabel(GetBoundKey(Spec.KeyboardMapping)),
			Spec.GamepadMapping.IsNone() ? FText::GetEmpty() : KeyLabel(GamepadKey),
			GamepadKey);
	}
}

void UJamKitOptionsWidget::EndCapture()
{
	CaptureRow.Reset();
	HintText->SetText(DefaultHint());
	RefreshRows();
}

void UJamKitOptionsWidget::HandleResetClicked()
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

void UJamKitOptionsWidget::HandleBackClicked()
{
	Close();
}

void UJamKitOptionsWidget::Close()
{
	EndCapture();
	if (UJamKitSettingsSubsystem* Settings = UJamKitSettingsSubsystem::Get(this))
	{
		Settings->SaveSettings();
	}
	Super::Close();
}

#undef LOCTEXT_NAMESPACE
