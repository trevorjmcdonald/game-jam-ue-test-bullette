#pragma once

#include "CoreMinimal.h"
#include "UI/JamKitMenuWidget.h"
#include "JamKitCreditsWidget.generated.h"

class UTextBlock;

/**
 * Credits: one line per entry of UJamKitSettings::Credits, then Back. Opened from a CreditsButton.
 * Layout lives in /JamKit/UI/WBP_Credits; these widget names must match.
 */
UCLASS(Abstract)
class JAMKIT_API UJamKitCreditsWidget : public UJamKitMenuWidget
{
	GENERATED_BODY()

public:
	virtual UWidget* GetDefaultFocusWidget() const override;
	virtual void OnOpened() override;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CreditsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton;

private:
	UFUNCTION()
	void HandleBackClicked();
};
