#pragma once

#include "CoreMinimal.h"
#include "UI/JamKitMenuWidget.h"
#include "JamKitMainMenuWidget.generated.h"

class UTextBlock;

/**
 * Title screen: Play opens UJamKitSettings::GameLevel; Options, Credits and Quit. TitleText shows
 * UJamKitSettings::GameTitle. Layout lives in /JamKit/UI/WBP_MainMenu; these widget names must match.
 */
UCLASS(Abstract)
class JAMKIT_API UJamKitMainMenuWidget : public UJamKitMenuWidget
{
	GENERATED_BODY()

public:
	virtual UWidget* GetDefaultFocusWidget() const override;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PlayButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

private:
	UFUNCTION()
	void HandlePlayClicked();

	UFUNCTION()
	void HandleQuitClicked();
};
