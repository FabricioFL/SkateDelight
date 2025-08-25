#pragma once

#include "CoreMinimal.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Slate widget to display the player's score in real-time.
 */
class SKATEDELIGHT_API SScoreHud : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SScoreHud) {}
    SLATE_END_ARGS()

    /** Constructs the HUD widget. */
    void Construct(const FArguments& InArgs);

    /** Updates the displayed score. */
    void UpdateScore(int32 NewScore);

private:
    /** Text block to display the score. */
    TSharedPtr<STextBlock> ScoreText;
};