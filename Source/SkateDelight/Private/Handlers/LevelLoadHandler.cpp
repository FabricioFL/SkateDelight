#include "Handlers/LevelLoadHandler.h"
#include "UI/MainMenu.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

void ULevelLoadHandler::StartLevelStreaming(const FName& LevelName, TWeakPtr<MainMenu> MenuWidget)
{
    LoadedLevelName = LevelName;
    TargetMenuWidget = MenuWidget;

    if (UWorld* World = GWorld)
    {
        UE_LOG(LogTemp, Log, TEXT("Starting async level streaming with FStreamableManager for: %s"), *LevelName.ToString());

        FString LevelPath = FString::Printf(TEXT("/Game/%s"), *LevelName.ToString());
        TSoftObjectPtr<UWorld> LevelAsset(LevelPath);

        if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
        {
            FStreamableManager& Streamable = AssetManager->GetStreamableManager();
            StreamableHandle = Streamable.RequestAsyncLoad(
                LevelAsset.ToSoftObjectPath(),
                FStreamableDelegate::CreateUObject(this, &ULevelLoadHandler::OnLevelLoaded),
                FStreamableManager::AsyncLoadHighPriority,
                true
            );

            if (!StreamableHandle.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to start async load for level: %s"), *LevelName.ToString());
                if (TSharedPtr<MainMenu> Menu = TargetMenuWidget.Pin())
                {
                    Menu->OnLevelLoadFailed();
                }
                return;
            }

            World->GetTimerManager().SetTimer(ProgressTimerHandle, this, &ULevelLoadHandler::UpdateLoadingProgress, 0.033f, true);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("AssetManager not initialized"));
            if (TSharedPtr<MainMenu> Menu = TargetMenuWidget.Pin())
            {
                Menu->OnLevelLoadFailed();
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ULevelLoadHandler: World not found"));
        if (TSharedPtr<MainMenu> Menu = TargetMenuWidget.Pin())
        {
            Menu->OnLevelLoadFailed();
        }
    }
}

void ULevelLoadHandler::OnLevelLoaded()
{
    if (UWorld* World = GWorld)
    {
        World->GetTimerManager().ClearTimer(ProgressTimerHandle);
        StreamableHandle.Reset();
        UE_LOG(LogTemp, Log, TEXT("Level streaming completed for: %s"), *LoadedLevelName.ToString());
    }

    if (TSharedPtr<MainMenu> Menu = TargetMenuWidget.Pin())
    {
        Menu->OnLevelLoaded(LoadedLevelName);
    }
}

void ULevelLoadHandler::UpdateLoadingProgress()
{
    if (TSharedPtr<MainMenu> Menu = TargetMenuWidget.Pin())
    {
        SimulatedProgress = FMath::Min(SimulatedProgress + 0.066f, 1.0f);
        UE_LOG(LogTemp, Log, TEXT("Updating progress: %.2f"), SimulatedProgress);
        Menu->OnProgressUpdated(SimulatedProgress);
    }
}