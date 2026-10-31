#pragma once

#include "CoreMinimal.h"
#include "UI/BulletteMenuWidget.h"
#include "BulletteMainMenuWidget.generated.h"

/** Title screen: Play opens GameLevel, Quit exits. Layout lives in WBP_MainMenu; these widget names must match. */
UCLASS(Abstract)
class BULLETTE_API UBulletteMainMenuWidget : public UBulletteMenuWidget
{
	GENERATED_BODY()

public:
	virtual UButton* GetDefaultFocusButton() const override { return PlayButton; }

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TSoftObjectPtr<UWorld> GameLevel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PlayButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

private:
	UFUNCTION()
	void HandlePlayClicked();

	UFUNCTION()
	void HandleQuitClicked();
};
