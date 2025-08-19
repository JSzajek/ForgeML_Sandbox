#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "NPCDefines.h"

#include <unordered_set>

#include "BaseDungeonActor.generated.h"


UCLASS(Abstract)
class ABaseDungeonActor : public AActor
{
	GENERATED_BODY()
public:
	ABaseDungeonActor();
public:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

	virtual void ResetActor(const FVector& location);

	virtual void OnFoundCoin() { }
	virtual void OnFoundTreasure() { }
	virtual void OnDeath() { }
protected:
	void MoveInDirection(EMoveDirection Direction,
						 float DeltaTime);
private:
	UFUNCTION()
	virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, 
								AActor* OtherActor, 
								UPrimitiveComponent* OtherComp, 
								int32 OtherBodyIndex, 
								bool bFromSweep, 
								const FHitResult& SweepResult);
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Movement")
    float mMoveSpeed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Movement")
    float mTimeBetweenDirectionSwap_s = 10;

    UPROPERTY(VisibleAnywhere)
    class UCapsuleComponent* mpCollisionComponent;

    UPROPERTY(VisibleAnywhere)
    class UStaticMeshComponent* mpMeshComponent;
protected:
	float mTime_s = 0;

	std::unordered_set<AActor*> mVisitedCoins;
};


