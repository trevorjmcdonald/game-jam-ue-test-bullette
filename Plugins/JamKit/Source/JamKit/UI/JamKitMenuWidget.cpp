#include "UI/JamKitMenuWidget.h"

#include "Audio/JamKitAudio.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "JamKit.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/JamKitFadeWidget.h"

void UJamKitMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		AddFocusTarget(Widget);
	});

	if (OptionsButton)
	{
		OptionsButton->OnClicked.AddDynamic(this, &UJamKitMenuWidget::HandleOptionsClicked);
	}
	if (CreditsButton)
	{
		CreditsButton->OnClicked.AddDynamic(this, &UJamKitMenuWidget::HandleCreditsClicked);
	}
}

void UJamKitMenuWidget::AddFocusTarget(UWidget* Widget)
{
	if (UButton* Button = Cast<UButton>(Widget))
	{
		FocusTargets.Add({ Button, Button->GetBackgroundColor() });
		Button->OnClicked.AddUniqueDynamic(this, &UJamKitMenuWidget::HandleAnyButtonClicked);
	}
	else if (USlider* Slider = Cast<USlider>(Widget))
	{
		FocusTargets.Add({ Slider, Slider->GetSliderHandleColor() });
	}
}

void UJamKitMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UWidget* Focused = nullptr;
	for (FFocusTarget& Target : FocusTargets)
	{
		UWidget* Widget = Target.Widget.Get();
		if (!Widget || !Widget->IsVisible())
		{
			continue;
		}

		if (Widget->IsHovered() && !Widget->HasAnyUserFocus())
		{
			Widget->SetKeyboardFocus();
		}

		const bool bFocused = Widget->HasAnyUserFocus();
		if (bFocused)
		{
			Focused = Widget;
		}
		if (bFocused != Target.bShowingFocus)
		{
			Target.bShowingFocus = bFocused;
			const FLinearColor Color = bFocused ? FocusedColor : Target.NormalColor;
			if (UButton* Button = Cast<UButton>(Widget))
			{
				Button->SetBackgroundColor(Color);
			}
			else if (USlider* Slider = Cast<USlider>(Widget))
			{
				Slider->SetSliderHandleColor(Color);
			}
		}
	}

	// Only a move between widgets makes a sound; the first focus when the screen opens doesn't.
	if (Focused && LastFocused.IsValid() && Focused != LastFocused.Get())
	{
		JamKitAudio::PlaySound(this, FocusSound, true, 0.0f);
	}
	LastFocused = Focused;
}

FReply UJamKitMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (OnClosed.IsBound() && BackKeys.Contains(InKeyEvent.GetKey()))
	{
		Close();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UJamKitMenuWidget::HandleAnyButtonClicked()
{
	JamKitAudio::PlaySound(this, ClickSound, true, 0.0f);
}

void UJamKitMenuWidget::OpenLevel(const TSoftObjectPtr<UWorld>& Level) const
{
	UJamKitFadeWidget::OpenLevelWithFade(GetOwningPlayer(), Level);
}

void UJamKitMenuWidget::QuitGame() const
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UJamKitMenuWidget::Close()
{
	OnClosed.ExecuteIfBound();
}

void UJamKitMenuWidget::HandleOptionsClicked()
{
	OpenSubScreen(OptionsScreenClass, OptionsButton);
}

void UJamKitMenuWidget::HandleCreditsClicked()
{
	OpenSubScreen(CreditsScreenClass, CreditsButton);
}

void UJamKitMenuWidget::OpenSubScreen(TSubclassOf<UJamKitMenuWidget> ScreenClass, UButton* ReturnFocusTo)
{
	if (!ScreenClass)
	{
		UE_LOG(LogJamKit, Warning, TEXT("%s: no screen class set for that button"), *GetName());
		return;
	}

	TObjectPtr<UJamKitMenuWidget>& Screen = SubScreens.FindOrAdd(ScreenClass);
	if (!Screen)
	{
		Screen = CreateWidget<UJamKitMenuWidget>(GetOwningPlayer(), ScreenClass);
		Screen->OnClosed.BindUObject(this, &UJamKitMenuWidget::HandleSubScreenClosed);
	}
	OpenScreen = Screen;
	ReturnFocus = ReturnFocusTo;

	// Collapse this screen so gamepad navigation can't wander back into it underneath.
	VisibilityBeforeSubScreen = GetVisibility();
	SetVisibility(ESlateVisibility::Collapsed);
	Screen->OnOpened();
	Screen->AddToViewport(50);
	if (UWidget* Focus = Screen->GetDefaultFocusWidget())
	{
		Focus->SetKeyboardFocus();
	}
}

void UJamKitMenuWidget::HandleSubScreenClosed()
{
	if (OpenScreen)
	{
		OpenScreen->RemoveFromParent();
		OpenScreen = nullptr;
	}
	SetVisibility(VisibilityBeforeSubScreen);
	if (UButton* Button = ReturnFocus.Get())
	{
		Button->SetKeyboardFocus();
	}
	else if (UWidget* Focus = GetDefaultFocusWidget())
	{
		Focus->SetKeyboardFocus();
	}
}
