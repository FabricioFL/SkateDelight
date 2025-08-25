#include "Actors/JumpScoreZone.h"
#include "Components/BoxComponent.h"
#include "Actors/APlayer.h"
#include "Kismet/GameplayStatics.h"

#define LOG_SCOREZONE(Format, ...) UE_LOG(LogTemp, Log, TEXT("JumpScoreZone: " Format), ##__VA_ARGS__)

AJumpScoreZone::AJumpScoreZone()
{
    PrimaryActorTick.bCanEverTick = false;

    ZoneBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBox"));
    RootComponent = ZoneBox;
    ZoneBox->SetCollisionProfileName(TEXT("Trigger"));
    ZoneBox->OnComponentBeginOverlap.AddDynamic(this, &AJumpScoreZone::OnOverlapBegin);
    ZoneBox->OnComponentEndOverlap.AddDynamic(this, &AJumpScoreZone::OnOverlapEnd);
}

void AJumpScoreZone::BeginPlay()
{
    Super::BeginPlay();

    // Explicitly set collision settings in BeginPlay to ensure they take effect
    ZoneBox->SetGenerateOverlapEvents(true);
    ZoneBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    LOG_SCOREZONE("Initialized at %s with GenerateOverlapEvents=%d", *GetActorLocation().ToString(), ZoneBox->GetGenerateOverlapEvents() ? 1 : 0);
}

void AJumpScoreZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    LOG_SCOREZONE("OverlapBegin detected with actor %s at %s", *OtherActor->GetName(), *OtherActor->GetActorLocation().ToString());

    if (AAPlayer* Player = Cast<AAPlayer>(OtherActor))
    {
        LOG_SCOREZONE("Player %s entered zone at %s", *Player->GetName(), *Player->GetActorLocation().ToString());
        OverlappingPlayer = Player;
        BindPlayerJump(true);
    }
    else
    {
        LOG_SCOREZONE("Non-player actor %s overlapped", *OtherActor->GetName());
    }
}

void AJumpScoreZone::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    LOG_SCOREZONE("OverlapEnd detected with actor %s", *OtherActor->GetName());

    if (AAPlayer* Player = Cast<AAPlayer>(OtherActor))
    {
        if (Player == OverlappingPlayer)
        {
            LOG_SCOREZONE("Player %s exited zone", *Player->GetName());
            BindPlayerJump(false);
            OverlappingPlayer = nullptr;
        }
    }
}

void AJumpScoreZone::BindPlayerJump(bool bBind)
{
    if (!OverlappingPlayer)
    {
        LOG_SCOREZONE("BindPlayerJump: No overlapping player");
        return;
    }

    if (bBind && !bIsJumpDelegateBound)
    {
        OverlappingPlayer->OnPlayerJumped.AddDynamic(this, &AJumpScoreZone::HandlePlayerJump);
        bIsJumpDelegateBound = true;
        LOG_SCOREZONE("Bound to OnPlayerJumped for player %s", *OverlappingPlayer->GetName());
    }
    else if (!bBind && bIsJumpDelegateBound)
    {
        OverlappingPlayer->OnPlayerJumped.RemoveDynamic(this, &AJumpScoreZone::HandlePlayerJump);
        bIsJumpDelegateBound = false;
        LOG_SCOREZONE("Unbound from OnPlayerJumped for player %s", *OverlappingPlayer->GetName());
    }
}

void AJumpScoreZone::HandlePlayerJump()
{
    LOG_SCOREZONE("HandlePlayerJump called");

    if (OverlappingPlayer && OverlappingPlayer->bIsRidingSkate && ZoneBox->IsOverlappingActor(OverlappingPlayer))
    {
        OverlappingPlayer->AddScore(PointsOnJump);
        LOG_SCOREZONE("Player jumped while mounted in zone, awarded %d points", PointsOnJump);

        APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC)
        {
            PC->ClientMessage(FString::Printf(TEXT("+%d Points!"), PointsOnJump));
        }
    }
    else
    {
        LOG_SCOREZONE("Jump ignored: OverlappingPlayer=%d, Mounted=%d, IsOverlapping=%d",
            OverlappingPlayer != nullptr ? 1 : 0,
            OverlappingPlayer ? OverlappingPlayer->bIsRidingSkate : false,
            OverlappingPlayer && ZoneBox->IsOverlappingActor(OverlappingPlayer) ? 1 : 0);
    }
}