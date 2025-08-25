#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SlateBasics.h"
#include "AMainMenu.generated.h"

class MainMenu;

UCLASS()
class SKATEDELIGHT_API AMainMenu : public AActor
{
    GENERATED_BODY()

public:
    AMainMenu();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    TSharedPtr<MainMenu> MainMenuWidget;
    TSharedPtr<SWeakWidget> ViewportWidgetContent;
};