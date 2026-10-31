#include "UI/BulletteMenuWidget.h"

#include "Audio/BulletteAudio.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UBulletteMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		if (UButton* Button = Cast<UButton>(Widget))
		{
			Buttons.Add({ Button, Button->GetBackgroundColor() });
			Button->OnClicked.AddDynamic(this, &UBulletteMenuWidget::HandleAnyButtonClicked);
		}
	});
}

void UBulletteMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UButton* FocusedButton = nullptr;
	for (FMenuButton& Entry : Buttons)
	{
		UButton* Button = Entry.Button.Get();
		if (!Button)
		{
			continue;
		}

		if (Button->IsHovered() && !Button->HasAnyUserFocus())
		{
			Button->SetKeyboardFocus();
		}

		const bool bFocused = Button->HasAnyUserFocus();
		if (bFocused)
		{
			FocusedButton = Button;
		}
		if (bFocused != Entry.bShowingFocus)
		{
			Entry.bShowingFocus = bFocused;
			Button->SetBackgroundColor(bFocused ? FocusedColor : Entry.NormalColor);
		}
	}

	// Only a move between buttons makes a sound; the first focus when the menu opens doesn't.
	if (FocusedButton && LastFocusedButton.IsValid() && FocusedButton != LastFocusedButton.Get())
	{
		BulletteAudio::PlaySound(this, FocusSound, true, 0.0f);
	}
	LastFocusedButton = FocusedButton;
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
