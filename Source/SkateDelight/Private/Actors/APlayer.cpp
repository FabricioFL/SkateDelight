// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/APlayer.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

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

	// --- Skate mesh component (always present) ---
	SkateMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SkateMesh"));
	if (SkateMeshComponent)
	{
		// default attachment to mesh (we'll reattach in BeginPlay if needed)
		SkateMeshComponent->SetupAttachment(GetMesh());
		SkateMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		SkateMeshComponent->SetSimulatePhysics(false);
		SkateMeshComponent->SetMobility(EComponentMobility::Movable);
		SkateMeshComponent->SetRelativeLocation(FVector::ZeroVector);
		SkateMeshComponent->SetRelativeRotation(FRotator::ZeroRotator);
		SkateMeshComponent->SetVisibility(true);
		SkateMeshComponent->SetHiddenInGame(false);
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
	bWantsAccelerate = false;
	bWantsBrake = false;
	Points = 0;
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

	// Assign a static mesh asset to the runtime component if the asset property is set
	if (SkateMeshComponent && SkateMeshAsset)
	{
		SkateMeshComponent->SetStaticMesh(SkateMeshAsset);
	}

	// Ensure movement params are applied
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		GetCharacterMovement()->JumpZVelocity = JumpForce;
	}

	// Start in hand
	AttachSkateToHand();
	bIsRidingSkate = false;
	CurrentSkateSpeed = 0.f;

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
	PlayerInputComponent->BindAction("SkateAccelerate", IE_Pressed, this, &AAPlayer::StartAccelerate);
	PlayerInputComponent->BindAction("SkateAccelerate", IE_Released, this, &AAPlayer::StopAccelerate);

	PlayerInputComponent->BindAction("SkateBrake", IE_Pressed, this, &AAPlayer::StartBrake);
	PlayerInputComponent->BindAction("SkateBrake", IE_Released, this, &AAPlayer::StopBrake);

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
void AAPlayer::StartAccelerate()
{
	LOG_SKATE("StartAccelerate pressed (riding=%d)", bIsRidingSkate ? 1 : 0);

	// If currently holding, mount the skate
	if (!bIsRidingSkate)
	{
		MountSkate();
	}
	bWantsAccelerate = true;
}

void AAPlayer::StopAccelerate()
{
	bWantsAccelerate = false;
	LOG_SKATE("StopAccelerate released");
}

void AAPlayer::StartBrake()
{
	bWantsBrake = true;
	LOG_SKATE("StartBrake pressed");
}

void AAPlayer::StopBrake()
{
	bWantsBrake = false;
	LOG_SKATE("StopBrake released");
}

// -----------------------------
// Jump
// -----------------------------
void AAPlayer::PerformJump()
{
	if (bIsRidingSkate)
	{
		const float SpeedThreshold = BaseSkateSpeed * 1.05f; // small tolerance
		if (CurrentSkateSpeed <= SpeedThreshold)
		{
			// zero jump (vertical)
			ACharacter::Jump();
			LOG_SKATE("Skate Zero Jump (speed=%.1f)", CurrentSkateSpeed);
		}
		else
		{
			// speed jump (directional)
			ACharacter::Jump();
			const FVector Fwd = GetActorForwardVector();
			LaunchCharacter(Fwd * 300.f, true, false);
			LOG_SKATE("Skate Speed Jump (speed=%.1f)", CurrentSkateSpeed);
		}

		// award points
		Points += 10;
		LOG_SKATE("Points=%d", Points);
	}
	else
	{
		ACharacter::Jump();
		LOG_SKATE("Normal Jump");
	}
}

// -----------------------------
// Mount/Dismount & movement
// -----------------------------
void AAPlayer::MountSkate()
{
	if (!SkateMeshComponent)
	{
		LOG_SKATE("MountSkate aborted: SkateMeshComponent == nullptr");
		return;
	}

	bIsRidingSkate = true;
	CurrentSkateSpeed = BaseSkateSpeed;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = CurrentSkateSpeed;
	}

	AttachSkateToRoot();

	// place skate visually under feet (customize in editor)
	SkateMeshComponent->SetRelativeLocation(SkateRidingRelativeLocation);
	SkateMeshComponent->SetRelativeRotation(SkateRidingRelativeRotation);

	LOG_SKATE("Mounted skate: speed=%.1f", CurrentSkateSpeed);
}

void AAPlayer::DismountSkate()
{
	if (!SkateMeshComponent)
	{
		LOG_SKATE("DismountSkate aborted: SkateMeshComponent == nullptr");
		return;
	}

	bIsRidingSkate = false;
	CurrentSkateSpeed = 0.f;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}

	AttachSkateToHand();

	// reset relative so socket snapping looks correct
	SkateMeshComponent->SetRelativeLocation(SkateHandRelativeLocation);
	SkateMeshComponent->SetRelativeRotation(SkateHandRelativeRotation);

	LOG_SKATE("Dismounted skate. Now walking (BaseWalkSpeed=%.1f)", BaseWalkSpeed);
}

void AAPlayer::HandleSkateMovement(float DeltaTime)
{
	if (!GetCharacterMovement())
		return;

	// accelerate
	if (bWantsAccelerate)
	{
		CurrentSkateSpeed = FMath::Clamp(
			CurrentSkateSpeed + SkateAccelRate * DeltaTime,
			BaseSkateSpeed,
			BaseSkateSpeed * MaxSkateSpeedMultiplier
		);
	}
	// brake
	else if (bWantsBrake)
	{
		CurrentSkateSpeed = FMath::Clamp(
			CurrentSkateSpeed - SkateDecelRate * DeltaTime,
			0.f,
			BaseSkateSpeed * MaxSkateSpeedMultiplier
		);

		if (CurrentSkateSpeed <= 0.f)
		{
			DismountSkate();
			return;
		}
	}
	else
	{
		// passive friction toward base speed
		CurrentSkateSpeed = FMath::FInterpTo(CurrentSkateSpeed, BaseSkateSpeed, DeltaTime, 1.0f);
	}

	// apply
	if (bIsRidingSkate && GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = CurrentSkateSpeed;
		LOG_SKATE("HandleSkateMovement: speed=%.1f", CurrentSkateSpeed);
	}
}

// -----------------------------
// Attach helpers (null-safe)
// -----------------------------
void AAPlayer::AttachSkateToHand()
{
	if (!SkateMeshComponent)
		return;

	SkateMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	if (USkeletalMeshComponent* CharMesh = GetMesh())
	{
		const FName HandSocket(TEXT("RightHandSocket"));
		if (CharMesh->DoesSocketExist(HandSocket))
		{
			SkateMeshComponent->AttachToComponent(CharMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocket);
			SkateMeshComponent->SetRelativeLocation(SkateHandRelativeLocation);
			SkateMeshComponent->SetRelativeRotation(SkateHandRelativeRotation);
			return;
		}
		// fallback: attach to mesh root
		SkateMeshComponent->AttachToComponent(CharMesh, FAttachmentTransformRules::KeepRelativeTransform);
		SkateMeshComponent->SetRelativeLocation(SkateHandRelativeLocation);
		SkateMeshComponent->SetRelativeRotation(SkateHandRelativeRotation);
	}
	else
	{
		// ultimate fallback: attach to root
		SkateMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		SkateMeshComponent->SetRelativeLocation(SkateHandRelativeLocation);
		SkateMeshComponent->SetRelativeRotation(SkateHandRelativeRotation);
	}
}

void AAPlayer::AttachSkateToRoot()
{
	if (!SkateMeshComponent)
		return;

	SkateMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	SkateMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
}
