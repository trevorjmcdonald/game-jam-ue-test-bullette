#include "UI/BulletteMainMenuWidget.h"

#include "Components/Button.h"

void UBulletteMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	PlayButton->OnClicked.AddDynamic(this, &UBulletteMainMenuWidget::HandlePlayClicked);
	QuitButton->OnClicked.AddDynamic(this, &UBulletteMainMenuWidget::HandleQuitClicked);
}

void UBulletteMainMenuWidget::HandlePlayClicked()
{
	OpenLevel(GameLevel);
}

void UBulletteMainMenuWidget::HandleQuitClicked()
{
	QuitGame();
}
