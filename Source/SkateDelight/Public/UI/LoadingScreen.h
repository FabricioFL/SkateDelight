#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SProgressBar.h"

class SKATEDELIGHT_API SLoadingScreen : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SLoadingScreen) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    void UpdateProgress(float Progress);

private:
    TSharedPtr<SProgressBar> LoadingProgressBar;
    TSharedPtr<STextBlock> LoadingText;
};