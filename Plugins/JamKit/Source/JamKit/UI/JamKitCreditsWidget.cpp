#include "UI/JamKitCreditsWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Settings/JamKitSettings.h"

UWidget* UJamKitCreditsWidget::GetDefaultFocusWidget() const
{
	return BackButton;
}

void UJamKitCreditsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BackButton->OnClicked.AddDynamic(this, &UJamKitCreditsWidget::HandleBackClicked);
}

void UJamKitCreditsWidget::OnOpened()
{
	CreditsText->SetText(FText::FromString(FString::Join(GetDefault<UJamKitSettings>()->Credits, TEXT("\n"))));
}

void UJamKitCreditsWidget::HandleBackClicked()
{
	Close();
}
