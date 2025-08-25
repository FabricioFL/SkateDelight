#include "Actors/AMainMenu.h"
#include "UI/MainMenu.h"
#include "SlateOptMacros.h"
#include "Widgets/SWeakWidget.h"
#include "Engine/Engine.h"

AMainMenu::AMainMenu()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AMainMenu::BeginPlay()
{
    Super::BeginPlay();

    if (GEngine && GEngine->GameViewport)
    {
        SAssignNew(MainMenuWidget, MainMenu);
        ViewportWidgetContent = SNew(SWeakWidget).PossiblyNullContent(MainMenuWidget);
        GEngine->GameViewport->AddViewportWidgetContent(ViewportWidgetContent.ToSharedRef());
    }
}

void AMainMenu::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GEngine && GEngine->GameViewport && ViewportWidgetContent.IsValid())
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(ViewportWidgetContent.ToSharedRef());
        ViewportWidgetContent.Reset();
        MainMenuWidget.Reset();
    }

    Super::EndPlay(EndPlayReason);
}