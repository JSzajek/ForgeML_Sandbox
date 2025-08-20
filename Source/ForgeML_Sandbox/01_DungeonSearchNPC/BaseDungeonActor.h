#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "NPCDefines.h"

#include <unordered_set>

#include "BaseDungeonActor.generated.h"


/// <summary>
/// Abstract base class for dungeon actors.
/// </summary>
UCLASS(Abstract)
class ABaseDungeonActor : public AActor
{
	GENERATED_BODY()
public:
	/// <summary>
	/// Constructor initializing a ABaseDungeonActor instance.
	/// </summary>
	ABaseDungeonActor();
public:
	/// <summary>
	/// Overridable native event for when play begins for this actor.
	/// </summary>
	virtual void BeginPlay() override;

	/// <summary>
	/// Overridable native event for when this actor is being destroyed.
	/// </summary>
	virtual void BeginDestroy() override;

	/// <summary>
	/// Resets the actor to a specific location.
	/// </summary>
	/// <param name="location">The new location to set</param>
	virtual void ResetActor(const FVector& location);

	/// <summary>
	/// Overridable native event called when the actor finds a coin.
	/// </summary>
	virtual void OnFoundCoin() { }

	/// <summary>
	/// Overridable native event called when the actor finds the treasure.
	/// </summary>
	virtual void OnFoundTreasure() { }

	/// <summary>
	/// Overridable native event called when the actor dies.
	/// </summary>
	virtual void OnDeath() { }
protected:
	/// <summary>
	/// Moves the actor in a specified direction based on the DeltaTime.
	/// </summary>
	/// <param name="direction">The direction</param>
	/// <param name="deltaTime">The deltatime</param>
	void MoveInDirection(EMoveDirection direction,
						 float deltaTime);
private:
	/// <summary>
	/// On BeginOverlap callback to handle when this actor overlaps with another actor.
	/// </summary>
	/// <param name="OverlappedComp">The overlapped component</param>
	/// <param name="OtherActor">The overlapped actor</param>
	/// <param name="OtherComp">The other component</param>
	/// <param name="OtherBodyIndex">The other body index</param>
	/// <param name="bFromSweep">Whether it was from a sweep</param>
	/// <param name="SweepResult">The sweep result</param>
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


