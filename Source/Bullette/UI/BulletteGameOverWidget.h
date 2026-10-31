#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BulletteGameOverWidget.generated.h"

class UButton;
class UTextBlock;

/** End-of-run summary with a restart button. Layout lives in WBP_GameOver; these widget names must match. */
UCLASS(Abstract)
class BULLETTE_API UBulletteGameOverWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowResults(const FText& Stats, const FText& Weapons);

	UButton* GetRestartButton() const { return RestartButton; }

	FSimpleDelegate OnRestart;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StatsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WeaponsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RestartButton;

private:
	UFUNCTION()
	void HandleRestartClicked();
};
