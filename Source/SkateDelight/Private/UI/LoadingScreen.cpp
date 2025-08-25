#include "UI/LoadingScreen.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SProgressBar.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLoadingScreen::Construct(const FArguments& InArgs)
{
    ChildSlot
        [
            SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FLinearColor(0.1f, 0.1f, 0.1f, 0.8f)) // Semi-transparent dark background
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                [
                    SNew(SVerticalBox)
                        // Loading text
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0, 0, 0, 20)
                        [
                            SAssignNew(LoadingText, STextBlock)
                                .Text(FText::FromString("Loading..."))
                                .Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 24))
                                .ColorAndOpacity(FLinearColor::White)
                                .Justification(ETextJustify::Center)
                        ]
                        // Progress bar wrapped in SBox for width control
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(SBox)
                                .WidthOverride(300.f) // Set desired width
                                [
                                    SAssignNew(LoadingProgressBar, SProgressBar)
                                        .Percent(0.f)
                                ]
                        ]
                ]
        ];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SLoadingScreen::UpdateProgress(float Progress)
{
    if (LoadingProgressBar.IsValid())
    {
        LoadingProgressBar->SetPercent(Progress);
    }
}