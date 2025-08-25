#include "UI/MainMenu.h"
#include "UI/LoadingScreen.h"
#include "Handlers/LevelLoadHandler.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Slate/SceneViewport.h"

#define SIDEBAR_WIDTH 0.2f

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void MainMenu::Construct(const FArguments& InArgs)
{
    ChildSlot
        [
            SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(0.25f)
                [
                    SNew(SBorder)
                        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .BorderBackgroundColor(FLinearColor::Black)
                        .Padding(0)
                        [
                            SNew(SVerticalBox)
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(30, 40, 30, 0)
                                [
                                    SNew(STextBlock)
                                        .Text(FText::FromString("Skate Delight"))
                                        .Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 32))
                                        .ColorAndOpacity(FLinearColor::White)
                                        .Justification(ETextJustify::Center)
                                ]
                                + SVerticalBox::Slot()
                                .FillHeight(0.3f)
                                [
                                    SNullWidget::NullWidget
                                ]
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .HAlign(HAlign_Center)
                                .Padding(0, 0, 0, 20)
                                [
                                    SNew(SVerticalBox)
                                        + SVerticalBox::Slot()
                                        .AutoHeight()
                                        .Padding(0, 0, 0, 15)
                                        [
                                            SNew(SButton)
                                                .OnClicked(this, &MainMenu::OnPlayClicked)
                                                .OnHovered(this, &MainMenu::OnPlayButtonHovered)
                                                .OnUnhovered(this, &MainMenu::OnPlayButtonUnhovered)
                                                .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                                                .ContentPadding(FMargin(20, 12))
                                                .Content()
                                                [
                                                    SAssignNew(PlayButtonText, STextBlock)
                                                        .Text(FText::FromString("Play"))
                                                        .Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 18))
                                                        .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
                                                        .Justification(ETextJustify::Left)
                                                ]
                                        ]
                                        + SVerticalBox::Slot()
                                        .AutoHeight()
                                        .Padding(0, 0, 0, 15)
                                        [
                                            SNew(SButton)
                                                .OnClicked(this, &MainMenu::OnExitClicked)
                                                .OnHovered(this, &MainMenu::OnExitButtonHovered)
                                                .OnUnhovered(this, &MainMenu::OnExitButtonUnhovered)
                                                .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                                                .ContentPadding(FMargin(20, 12))
                                                .Content()
                                                [
                                                    SAssignNew(ExitButtonText, STextBlock)
                                                        .Text(FText::FromString("Exit"))
                                                        .Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 18))
                                                        .ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f))
                                                        .Justification(ETextJustify::Left)
                                                ]
                                        ]
                                ]
                                + SVerticalBox::Slot()
                                .FillHeight(1.f)
                                [
                                    SNullWidget::NullWidget
                                ]
                        ]
                ]
                + SHorizontalBox::Slot()
                .FillWidth(0.75f)
                [
                    SNullWidget::NullWidget
                ]
        ];

    LockInputToUI();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FReply MainMenu::OnPlayClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Play button clicked! Starting level load..."));

    if (bIsLoading)
    {
        UE_LOG(LogTemp, Warning, TEXT("Already loading, ignoring click"));
        return FReply::Handled();
    }

    bIsLoading = true;
    LoadingTimeout = 0.0f;
    if (PlayButtonText.IsValid())
    {
        PlayButtonText->SetText(FText::FromString("Loading..."));
        PlayButtonText->SetColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
    }

    if (GEngine && GEngine->GameViewport)
    {
        LoadingScreenWidget = SNew(SLoadingScreen);
        GEngine->GameViewport->AddViewportWidgetContent(LoadingScreenWidget.ToSharedRef(), 1000);
        FSlateApplication::Get().Tick();
        UE_LOG(LogTemp, Log, TEXT("Loading screen added to viewport"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to add loading screen: GameViewport not found"));
        OnLevelLoadFailed();
        return FReply::Handled();
    }

    if (UWorld* World = GEngine ? GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr : nullptr)
    {
        UE_LOG(LogTemp, Log, TEXT("World found, creating LevelLoadHandler for level: Showcase"));
        ULevelLoadHandler* NewHandler = NewObject<ULevelLoadHandler>(World);
        LevelLoadHandler = NewHandler;
        NewHandler->StartLevelStreaming(FName("Showcase"), SharedThis(this));
        return FReply::Handled();
    }

    UE_LOG(LogTemp, Error, TEXT("Failed to start level streaming: World not found"));
    OnLevelLoadFailed();
    return FReply::Handled();
}

void MainMenu::OnLevelLoaded(const FName& LevelName)
{
    UE_LOG(LogTemp, Warning, TEXT("Level streaming completed, opening level: %s"), *LevelName.ToString());

    if (LoadingScreenWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(LoadingScreenWidget.ToSharedRef());
        LoadingScreenWidget.Reset();
        UE_LOG(LogTemp, Log, TEXT("Loading screen removed from viewport"));
    }

    if (GEngine && GEngine->GameViewport)
    {
        UWorld* World = GEngine->GameViewport->GetWorld();
        if (World && World->GetFirstPlayerController())
        {
            UE_LOG(LogTemp, Log, TEXT("Opening level: %s"), *LevelName.ToString());
            UGameplayStatics::OpenLevel(World, LevelName);
            bIsLoading = false;
            return;
        }
    }

    UE_LOG(LogTemp, Error, TEXT("Failed to open level: World or PlayerController not found"));
    OnLevelLoadFailed();
}

void MainMenu::OnLevelLoadFailed()
{
    UE_LOG(LogTemp, Error, TEXT("Level loading failed, resetting UI"));

    bIsLoading = false;
    LoadingTimeout = 0.0f;
    if (PlayButtonText.IsValid())
    {
        PlayButtonText->SetText(FText::FromString("Play"));
        PlayButtonText->SetColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f));
    }
    if (LoadingScreenWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(LoadingScreenWidget.ToSharedRef());
        LoadingScreenWidget.Reset();
        UE_LOG(LogTemp, Log, TEXT("Loading screen removed due to failure"));
    }
}

void MainMenu::OnProgressUpdated(float Progress)
{
    if (!bIsLoading || !LoadingScreenWidget.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Progress update ignored: Not loading or loading screen invalid"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Progress updated: %.2f"), Progress);
    LoadingScreenWidget->UpdateProgress(Progress);
}

void MainMenu::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

    if (bIsLoading)
    {
        LoadingTimeout += InDeltaTime;
        if (LoadingTimeout > 10.0f)
        {
            UE_LOG(LogTemp, Error, TEXT("Level loading timed out after %.2f seconds"), LoadingTimeout);
            OnLevelLoadFailed();
        }
    }
}

FReply MainMenu::OnExitClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("Exit button clicked!"));

    if (GEngine && GEngine->GameViewport)
    {
        UWorld* World = GEngine->GameViewport->GetWorld();
        if (World)
        {
            APlayerController* PC = World->GetFirstPlayerController();
            if (PC)
            {
                PC->ConsoleCommand("quit");
                FGenericPlatformMisc::RequestExit(false);
                return FReply::Handled();
            }
        }
    }

    FGenericPlatformMisc::RequestExit(false);
    return FReply::Handled();
}

void MainMenu::LockInputToUI()
{
    if (GEngine && GEngine->GameViewport)
    {
        UWorld* World = GEngine->GameViewport->GetWorld();
        if (World)
        {
            APlayerController* PC = World->GetFirstPlayerController();
            if (PC)
            {
                PC->bShowMouseCursor = true;
                PC->bEnableClickEvents = true;
                PC->bEnableMouseOverEvents = true;

                FInputModeUIOnly InputMode;
                InputMode.SetWidgetToFocus(SharedThis(this));
                InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                PC->SetInputMode(InputMode);

                UE_LOG(LogTemp, Warning, TEXT("Input locked to UI"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("PlayerController not found"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("World not found"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GEngine or GameViewport not found"));
    }
}

void MainMenu::OnPlayButtonHovered()
{
    if (PlayButtonText.IsValid() && !bIsLoading)
    {
        PlayButtonText->SetColorAndOpacity(FLinearColor::White);
    }
}

void MainMenu::OnPlayButtonUnhovered()
{
    if (PlayButtonText.IsValid() && !bIsLoading)
    {
        PlayButtonText->SetColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f));
    }
}

void MainMenu::OnExitButtonHovered()
{
    if (ExitButtonText.IsValid())
    {
        ExitButtonText->SetColorAndOpacity(FLinearColor::White);
    }
}

void MainMenu::OnExitButtonUnhovered()
{
    if (ExitButtonText.IsValid())
    {
        ExitButtonText->SetColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f, 1.0f));
    }
}