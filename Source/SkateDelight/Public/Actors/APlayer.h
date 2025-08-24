// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "APlayer.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UStaticMesh;

UCLASS()
class SKATEDELIGHT_API AAPlayer : public ACharacter
{
	GENERATED_BODY()

public:
	AAPlayer();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// -----------------------------
	// Tunable params (edit in editor)
	// -----------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Params")
	float BaseWalkSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Params")
	float BaseSkateSpeed = 600.f;

	// Max multiplier (2x -> set to 2.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Params")
	float MaxSkateSpeedMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Params")
	float SkateAccelRate = 500.f; // units per second^2-ish

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Params")
	float SkateDecelRate = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Params")
	float JumpForce = 600.f;

	// Points
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Params")
	int32 Points = 0;

	// -----------------------------
	// Components (created in C++)
	// -----------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera = nullptr;

	/** Static mesh component that shows the skate itself (always present as a component). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skate", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* SkateMeshComponent = nullptr;

	/** Asset you can drop into the actor defaults / blueprint to set the skateboard mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate")
	UStaticMesh* SkateMeshAsset = nullptr;

	/** Local transform offsets that are applied when attaching to hand or riding root (tweak in editor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate")
	FVector SkateHandRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate")
	FRotator SkateHandRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate")
	FVector SkateRidingRelativeLocation = FVector(30.f, 0.f, -90.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate")
	FRotator SkateRidingRelativeRotation = FRotator::ZeroRotator;

	// -----------------------------
	// Runtime state (read-only)
	// -----------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|State")
	bool bIsRidingSkate = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|State")
	float CurrentSkateSpeed = 0.f;

private:
	// Input state
	bool bWantsAccelerate = false;
	bool bWantsBrake = false;

	// -----------------------------
	// Input handlers
	// -----------------------------
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);

	void StartAccelerate(); // LShift pressed
	void StopAccelerate();  // LShift released
	void StartBrake();      // LCtrl pressed
	void StopBrake();       // LCtrl released

	void PerformJump();

	// -----------------------------
	// Skate helpers
	// -----------------------------
	void MountSkate();
	void DismountSkate();
	void HandleSkateMovement(float DeltaTime);

	// Attach helpers (null-safe)
	void AttachSkateToHand();
	void AttachSkateToRoot();
};
