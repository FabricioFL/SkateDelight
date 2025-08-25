#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

class SKATEDELIGHT_API MainMenu : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(MainMenu) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

    void OnLevelLoaded(const FName& LevelName);
    void OnLevelLoadFailed();
    void OnProgressUpdated(float Progress);

private:
    FReply OnPlayClicked();
    FReply OnExitClicked();
    void LockInputToUI();

    void OnPlayButtonHovered();
    void OnPlayButtonUnhovered();
    void OnExitButtonHovered();
    void OnExitButtonUnhovered();

    TSharedPtr<STextBlock> PlayButtonText;
    TSharedPtr<STextBlock> ExitButtonText;

    TSharedPtr<class SLoadingScreen> LoadingScreenWidget;

    bool bIsLoading = false;
    float LoadingTimeout = 0.0f;

    TWeakObjectPtr<class ULevelLoadHandler> LevelLoadHandler;
};