#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JumpScoreZone.generated.h"

class UBoxComponent;
class AAPlayer;

UCLASS()
class SKATEDELIGHT_API AJumpScoreZone : public AActor
{
    GENERATED_BODY()

public:
    AJumpScoreZone();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* ZoneBox;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoring")
    int32 PointsOnJump = 3;

    UPROPERTY(Transient)
    AAPlayer* OverlappingPlayer;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    // Bound to player delegate
    UFUNCTION()
    void HandlePlayerJump();

    void BindPlayerJump(bool bBind);

private:
    // Flag to track if delegate is bound to prevent duplicate bindings
    bool bIsJumpDelegateBound = false;
};