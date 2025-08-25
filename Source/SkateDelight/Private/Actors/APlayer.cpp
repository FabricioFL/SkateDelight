#include "Actors/APlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"
#include "UI/ScoreHud.h"
#include "SlateBasics.h"

#define LOG_SKATE(Format, ...) UE_LOG(LogTemp, Log, TEXT("Skate: " Format), ##__VA_ARGS__)

AAPlayer::AAPlayer()
{
    PrimaryActorTick.bCanEverTick = true;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 350.f;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 15.f;
    CameraBoom->bDoCollisionTest = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    SkateMountedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SkateMountedMesh"));
    if (SkateMountedMesh)
    {
        SkateMountedMesh->SetupAttachment(RootComponent);
        SkateMountedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SkateMountedMesh->SetSimulatePhysics(false);
        SkateMountedMesh->SetMobility(EComponentMobility::Movable);
        SkateMountedMesh->SetVisibility(false);
        SkateMountedMesh->SetHiddenInGame(false);
        SkateMountedMesh->SetRelativeLocation(FVector(30.f, 0.f, -90.f));
        SkateMountedMesh->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
    }

    SkateUnmountedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SkateUnmountedMesh"));
    if (SkateUnmountedMesh)
    {
        SkateUnmountedMesh->SetupAttachment(GetMesh());
        SkateUnmountedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SkateUnmountedMesh->SetSimulatePhysics(false);
        SkateUnmountedMesh->SetMobility(EComponentMobility::Movable);
        SkateUnmountedMesh->SetVisibility(true);
        SkateUnmountedMesh->SetHiddenInGame(false);
        SkateUnmountedMesh->SetRelativeLocation(FVector(-60.39f, 12.f, 180.f));
        SkateUnmountedMesh->SetRelativeRotation(FRotator(-40.50f, 0.f, 17.27f));
    }

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->bOrientRotationToMovement = true;
        GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
        GetCharacterMovement()->AirControl = 0.2f;
        GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
        GetCharacterMovement()->JumpZVelocity = JumpForce;
    }

    bIsRidingSkate = false;
    CurrentSkateSpeed = 0.f;
    bCanMove = true;
    LastSpeedupTime = 0.f;
    LastSlowdownTime = 0.f;
    LastJumpTime = 0.f;

    GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
}

void AAPlayer::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
    {
        if (PC->GetPawn() != this)
        {
            PC->Possess(this);
        }
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());

        ScoreHud = SNew(SScoreHud);
        GEngine->GameViewport->AddViewportWidgetContent(ScoreHud.ToSharedRef());
        ScoreHud->UpdateScore(Score);
        LOG_SKATE("ScoreHud created and added to viewport, initial score: %d", Score);
    }

    if (SkateMountedMesh && SkateMeshAsset)
    {
        SkateMountedMesh->SetStaticMesh(SkateMeshAsset);
        SkateMountedMesh->SetRelativeLocation(SkateMountedRelativeLocation);
        SkateMountedMesh->SetRelativeRotation(SkateMountedRelativeRotation);
    }
    if (SkateUnmountedMesh && SkateMeshAsset)
    {
        SkateUnmountedMesh->SetStaticMesh(SkateMeshAsset);
        SkateUnmountedMesh->SetRelativeLocation(SkateUnmountedRelativeLocation);
        SkateUnmountedMesh->SetRelativeRotation(SkateUnmountedRelativeRotation);
    }

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
        GetCharacterMovement()->JumpZVelocity = JumpForce;
    }

    SkateMountedMesh->SetVisibility(false);
    SkateUnmountedMesh->SetVisibility(true);
    bIsRidingSkate = false;
    CurrentSkateSpeed = 0.f;
    bCanMove = true;

    if (USkeletalMeshComponent* SkelMesh = GetMesh())
    {
        AnimInstance = SkelMesh->GetAnimInstance();
        LOG_SKATE("BeginPlay: AnimInstance initialized=%d", AnimInstance != nullptr);
        if (!AnimInstance)
        {
            LOG_SKATE("Error: AnimInstance is null. Check if skeletal mesh is properly configured.");
        }
    }

    if (IdleAnim && AnimInstance)
    {
        PlayAnimation(IdleAnim, true, false);
        CurrentAnimationState = TEXT("Idle");
        CurrentAnimEndTime = 0.f;
        LOG_SKATE("BeginPlay: Starting Idle animation");
    }
    else
    {
        LOG_SKATE("BeginPlay: Failed to start Idle animation. IdleAnim=%d, AnimInstance=%d",
            IdleAnim != nullptr, AnimInstance != nullptr);
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AAPlayer::UpdateAnimationState);
    }

    LOG_SKATE("BeginPlay: BaseWalk=%.1f SkateBase=%.1f", BaseWalkSpeed, BaseSkateSpeed);
}

void AAPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsRidingSkate && !GetCharacterMovement()->IsFalling())
    {
        HandleSkateMovement(DeltaTime);
    }

    UpdateAnimationState();
}

void AAPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis("MoveForward", this, &AAPlayer::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AAPlayer::MoveRight);
    PlayerInputComponent->BindAxis("Turn", this, &AAPlayer::Turn);
    PlayerInputComponent->BindAxis("LookUp", this, &AAPlayer::LookUp);
    PlayerInputComponent->BindAction("SkateAccelerate", IE_Pressed, this, &AAPlayer::AccelerateTap);
    PlayerInputComponent->BindAction("SkateBrake", IE_Pressed, this, &AAPlayer::BrakeTap);
    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AAPlayer::PerformJump);
}

void AAPlayer::MoveForward(float Value)
{
    if (Controller && Value != 0.f && bCanMove)
    {
        const FRotator ControlRot = Controller->GetControlRotation();
        const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);
        const FVector Dir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
        AddMovementInput(Dir, Value);
        LOG_SKATE("MoveForward: Value=%.2f", Value);
    }
}

void AAPlayer::MoveRight(float Value)
{
    if (Controller && Value != 0.f && bCanMove)
    {
        const FRotator ControlRot = Controller->GetControlRotation();
        const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);
        const FVector Dir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
        AddMovementInput(Dir, Value);
        LOG_SKATE("MoveRight: Value=%.2f", Value);
    }
}

void AAPlayer::Turn(float Value)
{
    AddControllerYawInput(Value);
}

void AAPlayer::LookUp(float Value)
{
    AddControllerPitchInput(Value);
}

void AAPlayer::AccelerateTap()
{
    if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
    {
        LOG_SKATE("AccelerateTap: Ignored due to Jump (falling)");
        return;
    }
    if (CurrentAnimationState == TEXT("Jump"))
    {
        LOG_SKATE("AccelerateTap: Ignored due to Jump animation");
        return;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentAnimationState == TEXT("Speedup") && CurrentTime < LastSpeedupTime + 2.367f) // 2.167s + 0.2s blend
    {
        if (bIsRidingSkate)
        {
            CurrentSkateSpeed = FMath::Clamp(
                CurrentSkateSpeed + SkateAccelBurst,
                BaseSkateSpeed,
                BaseSkateSpeed * MaxSkateSpeedMultiplier
            );
            LOG_SKATE("AccelerateTap: Queued speed increase (speed=%.1f, anim still playing)", CurrentSkateSpeed);
        }
        return;
    }

    if (!bIsRidingSkate)
    {
        MountSkate();
    }
    else
    {
        CurrentSkateSpeed = FMath::Clamp(
            CurrentSkateSpeed + SkateAccelBurst,
            BaseSkateSpeed,
            BaseSkateSpeed * MaxSkateSpeedMultiplier
        );
        if (SpeedupAnim && AnimInstance)
        {
            PlayAnimation(SpeedupAnim, false, true);
            CurrentAnimationState = TEXT("Speedup");
            LastSpeedupTime = CurrentTime;
        }
        LOG_SKATE("AccelerateTap: speed=%.1f", CurrentSkateSpeed);
    }
}

void AAPlayer::BrakeTap()
{
    if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
    {
        LOG_SKATE("BrakeTap: Ignored due to Jump (falling)");
        return;
    }
    if (CurrentAnimationState == TEXT("Jump"))
    {
        LOG_SKATE("BrakeTap: Ignored due to Jump animation");
        return;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentAnimationState == TEXT("Slowdown") && CurrentTime < LastSlowdownTime + 2.367f) // 2.167s + 0.2s blend
    {
        if (bIsRidingSkate)
        {
            CurrentSkateSpeed = FMath::Clamp(
                CurrentSkateSpeed - SkateDecelBurst,
                0.f,
                BaseSkateSpeed * MaxSkateSpeedMultiplier
            );
            if (CurrentSkateSpeed <= 0.f)
            {
                DismountSkate();
            }
            LOG_SKATE("BrakeTap: Queued speed decrease (speed=%.1f, anim still playing)", CurrentSkateSpeed);
        }
        return;
    }

    if (bIsRidingSkate)
    {
        CurrentSkateSpeed = FMath::Clamp(
            CurrentSkateSpeed - SkateDecelBurst,
            0.f,
            BaseSkateSpeed * MaxSkateSpeedMultiplier
        );
        if (CurrentSkateSpeed <= 0.f)
        {
            DismountSkate();
        }
        else
        {
            if (SlowdownAnim && AnimInstance)
            {
                PlayAnimation(SlowdownAnim, false, true);
                CurrentAnimationState = TEXT("Slowdown");
                LastSlowdownTime = CurrentTime;
            }
        }
        LOG_SKATE("BrakeTap: speed=%.1f", CurrentSkateSpeed);
    }
}

void AAPlayer::PerformJump()
{
    if (!GetCharacterMovement())
    {
        LOG_SKATE("PerformJump: No movement component");
        return;
    }

    ACharacter::Jump();

    const float BaseJumpZ = GetCharacterMovement()->JumpZVelocity;

    // Define forward & up vectors
    FVector Fwd = GetActorForwardVector();
    FVector Up = FVector::UpVector;
    Fwd.Normalize();

    // Boost factors
    float ForwardBoost = 400.f;   // base forward push
    float VerticalBoost = BaseJumpZ;

    // Interpolation factor based on speed
    float SpeedRatio = FMath::Clamp(CurrentSkateSpeed / (BaseSkateSpeed * 4.f), 0.f, 1.f);

    // Blending impulses
    FVector LowSpeedImpulse = Up * VerticalBoost;

    // Add stronger forward scaling at high speeds (×1.5)
    FVector HighSpeedImpulse = (Fwd * ForwardBoost * 3.5f) + (Up * VerticalBoost * 0.8f);

    FVector FinalImpulse = FMath::Lerp(LowSpeedImpulse, HighSpeedImpulse, SpeedRatio);

    // Apply impulse
    LaunchCharacter(FinalImpulse, true, true);

    LOG_SKATE("PerformJump: Speed=%.1f Ratio=%.2f Impulse=(%.1f, %.1f, %.1f)",
        CurrentSkateSpeed, SpeedRatio,
        FinalImpulse.X, FinalImpulse.Y, FinalImpulse.Z);

    // Animation override
    if (JumpAnim && AnimInstance)
    {
        PlayAnimation(JumpAnim, false, true);
        CurrentAnimationState = TEXT("Jump");
        bInPriorityAnimation = true;
    }

    OnPlayerJumped.Broadcast();

    // Reset anim priority when landing
    if (GetWorld())
    {
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(
            TimerHandle,
            [this]()
        {
            if (!GetCharacterMovement()->IsFalling())
            {
                bInPriorityAnimation = false;
            }
        },
            JumpAnim ? JumpAnim->GetPlayLength() + 0.2f : 0.8f,
            false
        );
    }
}


void AAPlayer::MountSkate()
{
    if (!SkateMountedMesh || !SkateUnmountedMesh)
    {
        LOG_SKATE("MountSkate aborted: Skate mesh components missing");
        return;
    }

    bIsRidingSkate = true;
    CurrentSkateSpeed = BaseSkateSpeed;
    bCanMove = true;

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = CurrentSkateSpeed;
    }

    SkateMountedMesh->SetVisibility(true);
    SkateUnmountedMesh->SetVisibility(false);
    SkateMountedMesh->SetRelativeLocation(SkateMountedRelativeLocation);
    SkateMountedMesh->SetRelativeRotation(SkateMountedRelativeRotation);

    if (MountAnim && AnimInstance)
    {
        PlayAnimation(MountAnim, false, true);
        CurrentAnimationState = TEXT("Mount");
    }
    else if (SkateboardingAnim && AnimInstance)
    {
        PlayAnimation(SkateboardingAnim, true, false);
        CurrentAnimationState = TEXT("Skateboarding");
    }

    LOG_SKATE("Mounted skate: speed=%.1f", CurrentSkateSpeed);
}

void AAPlayer::DismountSkate()
{
    if (!SkateMountedMesh || !SkateUnmountedMesh)
    {
        LOG_SKATE("DismountSkate aborted: Skate mesh components missing");
        return;
    }

    bIsRidingSkate = false;
    CurrentSkateSpeed = 0.f;
    bCanMove = false;

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
        GetCharacterMovement()->Velocity = FVector::ZeroVector;
    }

    SkateMountedMesh->SetVisibility(false);
    SkateUnmountedMesh->SetVisibility(true);
    SkateUnmountedMesh->SetRelativeLocation(SkateUnmountedRelativeLocation);
    SkateUnmountedMesh->SetRelativeRotation(SkateUnmountedRelativeRotation);

    if (DismountAnim && AnimInstance)
    {
        PlayAnimation(DismountAnim, false, true);
        CurrentAnimationState = TEXT("Dismount");
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(
            TimerHandle,
            [this]()
        {
            bCanMove = true;
            LOG_SKATE("Dismount: Movement re-enabled after animation");
            UpdateAnimationState();
        },
            0.62f,
            false
        );
    }
    else if (WalkingAnim && AnimInstance)
    {
        PlayAnimation(WalkingAnim, true, false);
        CurrentAnimationState = TEXT("Walking");
        bCanMove = true;
    }

    LOG_SKATE("Dismounted skate. Now walking (BaseWalkSpeed=%.1f, CanMove=%d)", BaseWalkSpeed, bCanMove ? 1 : 0);
}

void AAPlayer::HandleSkateMovement(float DeltaTime)
{
    if (!GetCharacterMovement())
        return;

    CurrentSkateSpeed = FMath::Max(0.f, CurrentSkateSpeed - FrictionDecelRate * DeltaTime);

    if (CurrentSkateSpeed <= 0.f && bIsRidingSkate)
    {
        DismountSkate();
        return;
    }

    if (bIsRidingSkate)
    {
        GetCharacterMovement()->MaxWalkSpeed = CurrentSkateSpeed;
        LOG_SKATE("HandleSkateMovement: speed=%.1f", CurrentSkateSpeed);
    }
}

void AAPlayer::PlayAnimation(UAnimSequence* AnimSequence, bool bLoop, bool bPriority)
{
    if (!AnimInstance || !AnimSequence)
    {
        LOG_SKATE("PlayAnimation failed: AnimInstance=%d, AnimSequence=%d", AnimInstance != nullptr, AnimSequence != nullptr);
        return;
    }

    const float BlendTime = 0.2f;
    GetMesh()->PlayAnimation(AnimSequence, bLoop);
    float PlayLength = (AnimSequence->GetPlayLength() / AnimSequence->RateScale) + (bLoop ? 0.f : BlendTime);
    CurrentAnimEndTime = bLoop ? 0.f : GetWorld()->GetTimeSeconds() + PlayLength;
    bInPriorityAnimation = bPriority;
    LOG_SKATE("Playing animation: %s, Loop=%d, Priority=%d, Duration=%.2f, RateScale=%.2f, BlendTime=%.2f",
        *AnimSequence->GetName(), bLoop, bPriority, PlayLength, AnimSequence->RateScale, BlendTime);
}

void AAPlayer::UpdateAnimationState()
{
    if (!AnimInstance || !GetCharacterMovement())
    {
        LOG_SKATE("UpdateAnimationState: AnimInstance=%d, CharacterMovement=%d",
            AnimInstance != nullptr, GetCharacterMovement() != nullptr);
        return;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentAnimEndTime > 0.f && CurrentTime >= CurrentAnimEndTime && CurrentAnimationState != TEXT("Jump"))
    {
        bInPriorityAnimation = false;
        CurrentAnimationState = NAME_None;
        LOG_SKATE("UpdateAnimationState: Animation %s finished, resetting state", *CurrentAnimationState.ToString());
    }

    if (GetCharacterMovement()->IsFalling())
    {
        if (JumpAnim && AnimInstance && CurrentAnimationState != TEXT("Jump"))
        {
            PlayAnimation(JumpAnim, false, true);
            CurrentAnimationState = TEXT("Jump");
            bInPriorityAnimation = true;
            LOG_SKATE("UpdateAnimationState: Transition to Jump (falling)");
        }
        return;
    }

    if (CurrentAnimationState == TEXT("Jump") && !GetCharacterMovement()->IsFalling())
    {
        bInPriorityAnimation = false;
        CurrentAnimationState = NAME_None;
        LOG_SKATE("UpdateAnimationState: Jump ended, forcing state transition");
    }

    if (bInPriorityAnimation && CurrentAnimationState != NAME_None)
    {
        LOG_SKATE("UpdateAnimationState: Waiting for priority animation %s (%.2f/%.2f)",
            *CurrentAnimationState.ToString(), CurrentTime, CurrentAnimEndTime);
        return;
    }

    float Speed = GetVelocity().Size2D();
    LOG_SKATE("UpdateAnimationState: Velocity=%.2f, Riding=%d, CanMove=%d",
        Speed, bIsRidingSkate ? 1 : 0, bCanMove ? 1 : 0);

    if (!bIsRidingSkate)
    {
        if (Speed > 5.f)
        {
            if (WalkingAnim && CurrentAnimationState != TEXT("Walking"))
            {
                PlayAnimation(WalkingAnim, true, false);
                CurrentAnimationState = TEXT("Walking");
                LOG_SKATE("UpdateAnimationState: Transition to Walking");
            }
        }
        else
        {
            if (IdleAnim && CurrentAnimationState != TEXT("Idle"))
            {
                PlayAnimation(IdleAnim, true, false);
                CurrentAnimationState = TEXT("Idle");
                LOG_SKATE("UpdateAnimationState: Transition to Idle");
            }
        }
    }
    else
    {
        if (SkateboardingAnim && CurrentAnimationState != TEXT("Skateboarding") &&
            CurrentAnimationState != TEXT("Speedup") && CurrentAnimationState != TEXT("Slowdown") &&
            CurrentAnimationState != TEXT("Jump") && CurrentAnimationState != TEXT("Mount") &&
            CurrentAnimationState != TEXT("Dismount"))
        {
            PlayAnimation(SkateboardingAnim, true, false);
            CurrentAnimationState = TEXT("Skateboarding");
            LOG_SKATE("UpdateAnimationState: Transition to Skateboarding");
        }
    }
}

void AAPlayer::AddScore(int32 Amount)
{
    const int32 Old = Score;
    Score = FMath::Max(0, Score + Amount);
    LOG_SKATE("AddScore: Amount=%d, Old=%d, New=%d", Amount, Old, Score);

    if (ScoreHud.IsValid())
    {
        ScoreHud->UpdateScore(Score);
    }
}