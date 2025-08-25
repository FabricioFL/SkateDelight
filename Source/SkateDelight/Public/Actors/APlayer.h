#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "APlayer.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UStaticMesh;
class SScoreHud;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerJumped);

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Params")
    float BaseWalkSpeed = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Params")
    float BaseSkateSpeed = 600.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Params")
    float MaxSkateSpeedMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Params")
    float SkateAccelBurst = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Params")
    float SkateDecelBurst = 150.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Params")
    float FrictionDecelRate = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Params")
    float JumpForce = 600.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skate", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* SkateMountedMesh = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skate", meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* SkateUnmountedMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate")
    UStaticMesh* SkateMeshAsset = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate")
    FVector SkateMountedRelativeLocation = FVector(30.f, 0.f, -90.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate")
    FRotator SkateMountedRelativeRotation = FRotator(0.f, 90.f, 0.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate")
    FVector SkateUnmountedRelativeLocation = FVector(-60.39f, 12.f, 180.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skate")
    FRotator SkateUnmountedRelativeRotation = FRotator(-40.50f, 0.f, 17.27f);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|State")
    bool bIsRidingSkate = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|State")
    float CurrentSkateSpeed = 0.f;

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    class UAnimSequence* MountAnim = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    class UAnimSequence* DismountAnim = nullptr;

    UPROPERTY(BlueprintAssignable, Category = "Player|Events")
    FOnPlayerJumped OnPlayerJumped;

    UFUNCTION(BlueprintCallable, Category = "Player|Score")
    void AddScore(int32 Amount);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Score")
    int32 Score = 0;

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void AccelerateTap();
    void BrakeTap();
    void PerformJump();
    void MountSkate();
    void DismountSkate();
    void HandleSkateMovement(float DeltaTime);
    void PlayAnimation(UAnimSequence* AnimSequence, bool bLoop = true, bool bPriority = false);
    void UpdateAnimationState();

    bool bWantsAccelerate = false;
    bool bWantsBrake = false;
    bool bCanMove = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
    class UAnimInstance* AnimInstance = nullptr;

    FName CurrentAnimationState = NAME_None;
    float CurrentAnimEndTime = 0.f;
    bool bInPriorityAnimation = false;

    TSharedPtr<class SScoreHud> ScoreHud;

    float LastSpeedupTime = 0.f;
    float LastSlowdownTime = 0.f;
    float LastJumpTime = 0.f;
};