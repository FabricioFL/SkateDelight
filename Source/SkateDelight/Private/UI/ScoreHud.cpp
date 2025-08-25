#include "UI/ScoreHud.h"
#include "SlateOptMacros.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SScoreHud::Construct(const FArguments& InArgs)
{
    ChildSlot
        [
            SNew(SBox)
                .Padding(FMargin(20.0f, 20.0f, 0.0f, 0.0f)) // Top-left corner with padding
                [
                    SAssignNew(ScoreText, STextBlock)
                        .Text(FText::FromString(TEXT("Score: 0")))
                        .Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 24))
                        .ColorAndOpacity(FLinearColor::White)
                        .ShadowOffset(FVector2D(1.0f, 1.0f))
                        .ShadowColorAndOpacity(FLinearColor::Black)
                ]
        ];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SScoreHud::UpdateScore(int32 NewScore)
{
    if (ScoreText.IsValid())
    {
        ScoreText->SetText(FText::FromString(FString::Printf(TEXT("Score: %d"), NewScore)));
    }
}