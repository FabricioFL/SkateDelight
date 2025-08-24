// Fill out your copyright notice in the Description page of Project Settings.

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

#define LOG_SKATE(Format, ...) UE_LOG(LogTemp, Log, TEXT("Skate: " Format), ##__VA_ARGS__)

AAPlayer::AAPlayer()
{
    PrimaryActorTick.bCanEverTick = true;

    // --- Camera boom (3rd person) ---
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 350.f;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 15.f;
    CameraBoom->bDoCollisionTest = true;

    // --- Camera ---
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // --- Skate mesh component for mounted state ---
    SkateMountedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SkateMountedMesh"));
    if (SkateMountedMesh)
    {
        SkateMountedMesh->SetupAttachment(RootComponent);
        SkateMountedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SkateMountedMesh->SetSimulatePhysics(false);
        SkateMountedMesh->SetMobility(EComponentMobility::Movable);
        SkateMountedMesh->SetVisibility(false);
        SkateMountedMesh->SetHiddenInGame(false);
    }

    // --- Skate mesh component for unmounted state ---
    SkateUnmountedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SkateUnmountedMesh"));
    if (SkateUnmountedMesh)
    {
        SkateUnmountedMesh->SetupAttachment(GetMesh());
        SkateUnmountedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SkateUnmountedMesh->SetSimulatePhysics(false);
        SkateUnmountedMesh->SetMobility(EComponentMobility::Movable);
        SkateUnmountedMesh->SetVisibility(true);
        SkateUnmountedMesh->SetHiddenInGame(false);
    }

    // --- Movement defaults ---
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->bOrientRotationToMovement = true;
        GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
        GetCharacterMovement()->AirControl = 0.2f;
        GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed; // set a safe default at construction time
        GetCharacterMovement()->JumpZVelocity = JumpForce;
    }

    // runtime state defaults
    bIsRidingSkate = false;
    CurrentSkateSpeed = 0.f;
    Points = 0;

    // Ensure skeletal mesh is set up for animations
    GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
}

void AAPlayer::BeginPlay()
{
    Super::BeginPlay();

    // Force possession of player 0 (keep behaviour)
    if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
    {
        if (PC->GetPawn() != this)
        {
            PC->Possess(this);
        }
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }

    // Assign a static mesh asset to the runtime components if the asset property is set
    if (SkateMountedMesh && SkateMeshAsset)
    {
        SkateMountedMesh->SetStaticMesh(SkateMeshAsset);
    }
    if (SkateUnmountedMesh && SkateMeshAsset)
    {
        SkateUnmountedMesh->SetStaticMesh(SkateMeshAsset);
    }

    // Ensure movement params are applied
    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
        GetCharacterMovement()->JumpZVelocity = JumpForce;
    }

    // Start unmounted with skate in hand
    SkateMountedMesh->SetVisibility(false);
    SkateUnmountedMesh->SetVisibility(true);
    bIsRidingSkate = false;
    CurrentSkateSpeed = 0.f;

    // Get the AnimInstance from the skeletal mesh
    if (USkeletalMeshComponent* SkelMesh = GetMesh())
    {
        AnimInstance = SkelMesh->GetAnimInstance();
    }

    // Start with idle animation
    if (IdleAnim && AnimInstance)
    {
        PlayAnimation(IdleAnim, true);
        CurrentAnimationState = TEXT("Idle");
    }

    LOG_SKATE("BeginPlay: BaseWalk=%.1f SkateBase=%.1f", BaseWalkSpeed, BaseSkateSpeed);
}

void AAPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // tick skate movement only when riding
    if (bIsRidingSkate)
    {
        HandleSkateMovement(DeltaTime);
    }

    UpdateAnimationState();
}

void AAPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // movement
    PlayerInputComponent->BindAxis("MoveForward", this, &AAPlayer::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &AAPlayer::MoveRight);

    // camera
    PlayerInputComponent->BindAxis("Turn", this, &AAPlayer::Turn);
    PlayerInputComponent->BindAxis("LookUp", this, &AAPlayer::LookUp);

    // skate actions (make sure your DefaultInput.ini maps these)
    PlayerInputComponent->BindAction("SkateAccelerate", IE_Pressed, this, &AAPlayer::AccelerateTap);

    PlayerInputComponent->BindAction("SkateBrake", IE_Pressed, this, &AAPlayer::BrakeTap);

    // jump
    PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AAPlayer::PerformJump);
}

// -----------------------------
// Movement input
// -----------------------------
void AAPlayer::MoveForward(float Value)
{
    if (Controller && Value != 0.f)
    {
        const FRotator ControlRot = Controller->GetControlRotation();
        const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);
        const FVector Dir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
        AddMovementInput(Dir, Value);
    }
}

void AAPlayer::MoveRight(float Value)
{
    if (Controller && Value != 0.f)
    {
        const FRotator ControlRot = Controller->GetControlRotation();
        const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);
        const FVector Dir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
        AddMovementInput(Dir, Value);
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

// -----------------------------
// Skate input handlers
// -----------------------------
void AAPlayer::AccelerateTap()
{
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
            PlayAnimation(SpeedupAnim, false);
            CurrentAnimationState = TEXT("Speedup");
        }
        LOG_SKATE("AccelerateTap: speed=%.1f", CurrentSkateSpeed);
    }
}

void AAPlayer::BrakeTap()
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
        if (SlowdownAnim && AnimInstance)
        {
            PlayAnimation(SlowdownAnim, false);
            CurrentAnimationState = TEXT("Slowdown");
        }
        LOG_SKATE("BrakeTap: speed=%.1f", CurrentSkateSpeed);
    }
    // No effect if unmounted
}

// -----------------------------
// Jump
// -----------------------------
void AAPlayer::PerformJump()
{
    if (bIsRidingSkate || !bIsRidingSkate)
    {
        const float SpeedThreshold = BaseSkateSpeed * 1.05f; // small tolerance
        if (CurrentSkateSpeed <= SpeedThreshold || !bIsRidingSkate)
        {
            // zero jump (vertical)
            ACharacter::Jump();
            LOG_SKATE("Zero Jump (speed=%.1f, riding=%d)", CurrentSkateSpeed, bIsRidingSkate ? 1 : 0);
        }
        else
        {
            // speed jump (directional)
            ACharacter::Jump();
            const FVector Fwd = GetActorForwardVector();
            LaunchCharacter(Fwd * 300.f, true, false);
            LOG_SKATE("Speed Jump (speed=%.1f, riding=%d)", CurrentSkateSpeed, bIsRidingSkate ? 1 : 0);
        }

        // award points
        Points += 10;
        LOG_SKATE("Points=%d", Points);
    }

    // Force jump animation
    if (JumpAnim && AnimInstance)
    {
        PlayAnimation(JumpAnim, false);
        CurrentAnimationState = TEXT("Jump");
    }
}

// -----------------------------
// Mount/Dismount & movement
// -----------------------------
void AAPlayer::MountSkate()
{
    if (!SkateMountedMesh || !SkateUnmountedMesh)
    {
        LOG_SKATE("MountSkate aborted: Skate mesh components missing");
        return;
    }

    bIsRidingSkate = true;
    CurrentSkateSpeed = BaseSkateSpeed;

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
        PlayAnimation(MountAnim, false);
        CurrentAnimationState = TEXT("Mount");
    }
    else if (SkateboardingAnim && AnimInstance)
    {
        PlayAnimation(SkateboardingAnim, true);
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

    if (GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
    }

    SkateMountedMesh->SetVisibility(false);
    SkateUnmountedMesh->SetVisibility(true);
    SkateUnmountedMesh->SetRelativeLocation(SkateUnmountedRelativeLocation);
    SkateUnmountedMesh->SetRelativeRotation(SkateUnmountedRelativeRotation);

    if (DismountAnim && AnimInstance)
    {
        PlayAnimation(DismountAnim, false);
        CurrentAnimationState = TEXT("Dismount");
    }
    else if (WalkingAnim && AnimInstance)
    {
        PlayAnimation(WalkingAnim, true);
        CurrentAnimationState = TEXT("Walking");
    }

    LOG_SKATE("Dismounted skate. Now walking (BaseWalkSpeed=%.1f)", BaseWalkSpeed);
}

void AAPlayer::HandleSkateMovement(float DeltaTime)
{
    if (!GetCharacterMovement())
        return;

    // Passive friction always when riding
    CurrentSkateSpeed = FMath::Max(0.f, CurrentSkateSpeed - FrictionDecelRate * DeltaTime);

    if (CurrentSkateSpeed <= 0.f && bIsRidingSkate)
    {
        DismountSkate();
        return;
    }

    // apply
    if (bIsRidingSkate && GetCharacterMovement())
    {
        GetCharacterMovement()->MaxWalkSpeed = CurrentSkateSpeed;
        LOG_SKATE("HandleSkateMovement: speed=%.1f", CurrentSkateSpeed);
    }
}

// -----------------------------
// Animation helpers
// -----------------------------
void AAPlayer::PlayAnimation(UAnimSequence* AnimSequence, bool bLoop)
{
    if (AnimInstance && AnimSequence)
    {
        GetMesh()->PlayAnimation(AnimSequence, bLoop);
    }
}

void AAPlayer::UpdateAnimationState()
{
    if (!AnimInstance || !GetCharacterMovement())
        return;

    if (GetCharacterMovement()->IsFalling())
    {
        if (JumpAnim && CurrentAnimationState != TEXT("Jump"))
        {
            PlayAnimation(JumpAnim, true);
            CurrentAnimationState = TEXT("Jump");
        }
        return;
    }

    float Speed = GetVelocity().Size2D();

    if (!bIsRidingSkate)
    {
        if (Speed > 10.f) // threshold for moving
        {
            if (WalkingAnim && CurrentAnimationState != TEXT("Walking"))
            {
                PlayAnimation(WalkingAnim, true);
                CurrentAnimationState = TEXT("Walking");
            }
        }
        else
        {
            if (IdleAnim && CurrentAnimationState != TEXT("Idle"))
            {
                PlayAnimation(IdleAnim, true);
                CurrentAnimationState = TEXT("Idle");
            }
        }
    }
    else
    {
        if (CurrentSkateSpeed > BaseSkateSpeed * 1.2f) // Example threshold for speedup anim
        {
            if (SpeedupAnim && CurrentAnimationState != TEXT("Speedup"))
            {
                PlayAnimation(SpeedupAnim, true);
                CurrentAnimationState = TEXT("Speedup");
            }
        }
        else if (CurrentSkateSpeed < BaseSkateSpeed * 0.8f) // Example threshold for slowdown anim
        {
            if (SlowdownAnim && CurrentAnimationState != TEXT("Slowdown"))
            {
                PlayAnimation(SlowdownAnim, true);
                CurrentAnimationState = TEXT("Slowdown");
            }
        }
        else
        {
            if (SkateboardingAnim && CurrentAnimationState != TEXT("Skateboarding"))
            {
                PlayAnimation(SkateboardingAnim, true);
                CurrentAnimationState = TEXT("Skateboarding");
            }
        }
    }
}