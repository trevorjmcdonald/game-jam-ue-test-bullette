#include "UI/BulletteMenuWidget.h"

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
		}
	});
}

void UBulletteMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

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
		if (bFocused != Entry.bShowingFocus)
		{
			Entry.bShowingFocus = bFocused;
			Button->SetBackgroundColor(bFocused ? FocusedColor : Entry.NormalColor);
		}
	}
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
