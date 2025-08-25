// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
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
    float SkateAccelBurst = 100.f; // Speed increase per tap

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Params")
    float SkateDecelBurst = 150.f; // Speed decrease per tap

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Params")
    float FrictionDecelRate = 100.f; // Passive deceleration per second

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Params")
    float JumpForce = 600.f;

    // -----------------------------
    // Components (created in C++)
    // -----------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera = nullptr;

    /** Static mesh component for skate when mounted (under feet). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skate", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* SkateMountedMesh = nullptr;

    /** Static mesh component for skate when unmounted (in hand). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skate", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* SkateUnmountedMesh = nullptr;

    /** Asset you can drop into the actor defaults / blueprint to set the skateboard mesh. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate")
    UStaticMesh* SkateMeshAsset = nullptr;

    /** Local transform offsets for mounted skate (tweak in editor). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate")
    FVector SkateMountedRelativeLocation = FVector(30.f, 0.f, -90.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate")
    FRotator SkateMountedRelativeRotation = FRotator(0.f, 90.f, 0.f); // Adjusted for 90 degrees on horizontal axis

    /** Local transform offsets for unmounted skate (tweak in editor). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate")
    FVector SkateUnmountedRelativeLocation = FVector(-60.39f, 12.f, 180.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate")
    FRotator SkateUnmountedRelativeRotation = FRotator(-40.50f, 0.f, 17.27f);

    // -----------------------------
    // Runtime state (read-only)
    // -----------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|State")
    bool bIsRidingSkate = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|State")
    float CurrentSkateSpeed = 0.f;

    // Animation sequences (assign in editor or load dynamically)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    class UAnimSequence* IdleAnim = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    class UAnimSequence* WalkingAnim = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    class UAnimSequence* SkateboardingAnim = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    class UAnimSequence* SpeedupAnim = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    class UAnimSequence* SlowdownAnim = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    class UAnimSequence* JumpAnim = nullptr;

    // Slots for more animations (e.g., mount, dismount, etc.)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    class UAnimSequence* MountAnim = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    class UAnimSequence* DismountAnim = nullptr;

private:
    // Input state
    bool bWantsAccelerate = false;
    bool bWantsBrake = false;

    // Animation
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    class UAnimInstance* AnimInstance = nullptr;

    // Animation state
    FName CurrentAnimationState = NAME_None;

    // To prevent cutting animations
    float CurrentAnimEndTime = 0.f;

    bool bInPriorityAnimation = false;

    // -----------------------------
    // Input handlers
    // -----------------------------
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);

    void AccelerateTap(); // LShift pressed for burst
    void BrakeTap();      // LCtrl pressed for burst

    void PerformJump();

    // -----------------------------
    // Skate helpers
    // -----------------------------
    void MountSkate();
    void DismountSkate();
    void HandleSkateMovement(float DeltaTime);

    // Animation helpers
    void PlayAnimation(UAnimSequence* AnimSequence, bool bLoop = true, bool bPriority = false);
    void UpdateAnimationState();
};