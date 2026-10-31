#pragma once

#include "CoreMinimal.h"
#include "UI/BulletteMenuWidget.h"
#include "BulletteMainMenuWidget.generated.h"

class UTextBlock;

/**
 * Title screen: Play opens GameLevel, Options opens the options screen, Quit exits. Shows the best run if there is one.
 * Layout lives in WBP_MainMenu; these widget names must match.
 */
UCLASS(Abstract)
class BULLETTE_API UBulletteMainMenuWidget : public UBulletteMenuWidget
{
	GENERATED_BODY()

public:
	virtual UWidget* GetDefaultFocusWidget() const override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TSoftObjectPtr<UWorld> GameLevel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PlayButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	/** "Best run 3:21 - 120 kills"; hidden until a run has been finished. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BestRunText;

private:
	UFUNCTION()
	void HandlePlayClicked();

	UFUNCTION()
	void HandleQuitClicked();
};
