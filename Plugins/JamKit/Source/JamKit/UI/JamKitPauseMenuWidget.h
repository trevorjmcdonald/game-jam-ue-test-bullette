#pragma once

#include "CoreMinimal.h"
#include "UI/JamKitMenuWidget.h"
#include "JamKitPauseMenuWidget.generated.h"

class UInputAction;

/**
 * Pause menu: Resume, Options, Main Menu (UJamKitSettings::MainMenuLevel), Quit. Layout lives in /JamKit/UI/WBP_PauseMenu;
 * these widget names must match. AJamKitPlayerController opens and closes it.
 * The menu takes UI-only input, so the gameplay Pause action can't fire while it's open. It resumes on ResumeKeys
 * and on whatever keys Pause is currently bound to (they can be rebound in the options screen).
 */
UCLASS(Abstract)
class JAMKIT_API UJamKitPauseMenuWidget : public UJamKitMenuWidget
{
	GENERATED_BODY()

public:
	virtual UWidget* GetDefaultFocusWidget() const override;

	/** The action whose current keys also close the menu. */
	void SetPauseAction(const UInputAction* InPauseAction) { PauseAction = InPauseAction; }

	FSimpleDelegate OnResume;

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Keys that close the menu, besides the Resume button and the Pause action's keys. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TArray<FKey> ResumeKeys = { EKeys::Escape, EKeys::Gamepad_FaceButton_Right };

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

	bool IsPauseKey(const FKey& Key) const;

	UPROPERTY()
	TObjectPtr<const UInputAction> PauseAction;
};
