#pragma once

#include "CoreMinimal.h"
#include "UI/BulletteMenuWidget.h"
#include "BullettePauseMenuWidget.generated.h"

/**
 * Pause menu: Resume, Main Menu, Quit. Layout lives in WBP_PauseMenu; these widget names must match.
 * The menu takes UI-only input, so the gameplay Pause action can't fire while it's open; it resumes on ResumeKeys itself.
 */
UCLASS(Abstract)
class BULLETTE_API UBullettePauseMenuWidget : public UBulletteMenuWidget
{
	GENERATED_BODY()

public:
	virtual UButton* GetDefaultFocusButton() const override { return ResumeButton; }

	FSimpleDelegate OnResume;

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TSoftObjectPtr<UWorld> MainMenuLevel;

	/** Keys that close the menu, besides the Resume button. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TArray<FKey> ResumeKeys = { EKeys::Escape, EKeys::P, EKeys::Gamepad_Special_Right, EKeys::Gamepad_FaceButton_Right };

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MainMenuButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

private:
	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleMainMenuClicked();

	UFUNCTION()
	void HandleQuitClicked();
};
