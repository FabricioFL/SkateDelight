#include "Actors/JumpScoreZone.h"
#include "Components/BoxComponent.h"
#include "Actors/APlayer.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

#define LOG_SCOREZONE(Format, ...) UE_LOG(LogTemp, Log, TEXT("JumpScoreZone: " Format), ##__VA_ARGS__)

AJumpScoreZone::AJumpScoreZone()
{
    PrimaryActorTick.bCanEverTick = false;

    ZoneBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBox"));
    RootComponent = ZoneBox;
    ZoneBox->SetCollisionProfileName(TEXT("Trigger"));
    ZoneBox->OnComponentBeginOverlap.AddDynamic(this, &AJumpScoreZone::OnOverlapBegin);
    ZoneBox->OnComponentEndOverlap.AddDynamic(this, &AJumpScoreZone::OnOverlapEnd);

    bWasAirborneOnEntry = false;
    bRemainedAirborne = true;
    bHasJumped = false;
}

void AJumpScoreZone::BeginPlay()
{
    Super::BeginPlay();

    // Explicitly set collision settings in BeginPlay to ensure they take effect
    ZoneBox->SetGenerateOverlapEvents(true);
    ZoneBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    LOG_SCOREZONE("Initialized at %s with GenerateOverlapEvents=%d", *GetActorLocation().ToString(), ZoneBox->GetGenerateOverlapEvents() ? 1 : 0);
}

void AJumpScoreZone::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (OverlappingPlayer)
    {
        bool bIsGrounded = OverlappingPlayer->GetCharacterMovement() ? OverlappingPlayer->GetCharacterMovement()->IsMovingOnGround() : true;
        if (bIsGrounded)
        {
            bRemainedAirborne = false;
            LOG_SCOREZONE("Player landed inside zone, bRemainedAirborne set to false");
        }
    }
    else
    {
        PrimaryActorTick.bCanEverTick = false;
    }
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
        bool bIsGrounded = Player->GetCharacterMovement() ? Player->GetCharacterMovement()->IsMovingOnGround() : true;
        bWasAirborneOnEntry = !bIsGrounded;
        bRemainedAirborne = true;
        bHasJumped = false;
        PrimaryActorTick.bCanEverTick = true;
        BindPlayerJump(true);
        LOG_SCOREZONE("Entry state: Airborne=%d", bWasAirborneOnEntry ? 1 : 0);
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
            bool bIsGrounded = Player->GetCharacterMovement() ? Player->GetCharacterMovement()->IsMovingOnGround() : true;
            LOG_SCOREZONE("Exit state: Airborne=%d, RemainedAirborne=%d, WasAirborneOnEntry=%d, HasJumped=%d, RidingSkate=%d",
                !bIsGrounded ? 1 : 0, bRemainedAirborne ? 1 : 0, bWasAirborneOnEntry ? 1 : 0, bHasJumped ? 1 : 0, Player->bIsRidingSkate ? 1 : 0);

            // New case: award points if entered airborne, remained airborne, exited airborne, and riding skate
            if (bWasAirborneOnEntry && bRemainedAirborne && !bIsGrounded && Player->bIsRidingSkate)
            {
                Player->AddScore(PointsOnJump);
                LOG_SCOREZONE("New case: Player awarded %d points for passing through zone airborne", PointsOnJump);

                APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
                if (PC)
                {
                    PC->ClientMessage(FString::Printf(TEXT("+%d Points!"), PointsOnJump));
                }
            }
            else
            {
                LOG_SCOREZONE("New case ignored: WasAirborneOnEntry=%d, RemainedAirborne=%d, AirborneOnExit=%d, Mounted=%d",
                    bWasAirborneOnEntry ? 1 : 0, bRemainedAirborne ? 1 : 0, !bIsGrounded ? 1 : 0, Player->bIsRidingSkate ? 1 : 0);
            }

            BindPlayerJump(false);
            OverlappingPlayer = nullptr;
            PrimaryActorTick.bCanEverTick = false;
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

    // Track jump for potential new case
    bHasJumped = true;
}