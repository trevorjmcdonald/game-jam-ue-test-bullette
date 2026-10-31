#include "UI/BulletteMenuWidget.h"

#include "Audio/BulletteAudio.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/BulletteOptionsWidget.h"

void UBulletteMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		AddFocusTarget(Widget);
	});

	if (OptionsButton)
	{
		OptionsButton->OnClicked.AddDynamic(this, &UBulletteMenuWidget::HandleOptionsClicked);
	}
}

void UBulletteMenuWidget::AddFocusTarget(UWidget* Widget)
{
	if (UButton* Button = Cast<UButton>(Widget))
	{
		FocusTargets.Add({ Button, Button->GetBackgroundColor() });
		Button->OnClicked.AddUniqueDynamic(this, &UBulletteMenuWidget::HandleAnyButtonClicked);
	}
	else if (USlider* Slider = Cast<USlider>(Widget))
	{
		FocusTargets.Add({ Slider, Slider->GetSliderHandleColor() });
	}
}

void UBulletteMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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

	// Only a move between widgets makes a sound; the first focus when the menu opens doesn't.
	if (Focused && LastFocused.IsValid() && Focused != LastFocused.Get())
	{
		BulletteAudio::PlaySound(this, FocusSound, true, 0.0f);
	}
	LastFocused = Focused;
}

void UBulletteMenuWidget::HandleAnyButtonClicked()
{
	BulletteAudio::PlaySound(this, ClickSound, true, 0.0f);
}

void UBulletteMenuWidget::OpenLevel(const TSoftObjectPtr<UWorld>& Level) const
{
	if (ensureMsgf(!Level.IsNull(), TEXT("%s has no level to open"), *GetName()))
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, Level);
	}
}

void UBulletteMenuWidget::QuitGame() const
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UBulletteMenuWidget::HandleOptionsClicked()
{
	OpenOptions();
}

void UBulletteMenuWidget::OpenOptions()
{
	if (!ensureMsgf(OptionsWidgetClass, TEXT("%s has no OptionsWidgetClass"), *GetName()))
	{
		return;
	}

	if (!OptionsWidget)
	{
		OptionsWidget = CreateWidget<UBulletteOptionsWidget>(GetOwningPlayer(), OptionsWidgetClass);
		OptionsWidget->OnClosed.BindUObject(this, &UBulletteMenuWidget::HandleOptionsClosed);
	}

	// Collapse this menu so gamepad navigation can't wander back into it underneath the options screen.
	VisibilityBeforeOptions = GetVisibility();
	SetVisibility(ESlateVisibility::Collapsed);
	OptionsWidget->RefreshFromSettings();
	OptionsWidget->AddToViewport(50);
	if (UWidget* Focus = OptionsWidget->GetDefaultFocusWidget())
	{
		Focus->SetKeyboardFocus();
	}
}

void UBulletteMenuWidget::HandleOptionsClosed()
{
	OptionsWidget->RemoveFromParent();
	SetVisibility(VisibilityBeforeOptions);
	if (OptionsButton)
	{
		OptionsButton->SetKeyboardFocus();
	}
}
