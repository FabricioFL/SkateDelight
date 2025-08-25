#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LevelLoadHandler.generated.h"

class MainMenu;
struct FStreamableHandle;

UCLASS()
class SKATEDELIGHT_API ULevelLoadHandler : public UObject
{
    GENERATED_BODY()

public:
    void StartLevelStreaming(const FName& LevelName, TWeakPtr<MainMenu> MenuWidget);

    UFUNCTION()
    void OnLevelLoaded();

private:
    UFUNCTION()
    void UpdateLoadingProgress();

    FName LoadedLevelName;
    TWeakPtr<MainMenu> TargetMenuWidget;
    FTimerHandle ProgressTimerHandle;
    float SimulatedProgress = 0.0f;
    TSharedPtr<FStreamableHandle> StreamableHandle;
};