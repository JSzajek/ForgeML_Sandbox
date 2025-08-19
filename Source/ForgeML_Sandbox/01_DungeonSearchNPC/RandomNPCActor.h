#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "NPCDefines.h"

#include <functional>

#include "RandomNPCActor.generated.h"

UCLASS()
class ARandomNPCActor : public AActor
{
	GENERATED_BODY()
public:
	ARandomNPCActor();
public:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
public:
	virtual void TickActor(float DeltaTime, 
						   enum ELevelTick TickType, 
						   FActorTickFunction& ThisTickFunction) override;
public:
	void MoveInDirection(EMoveDirection Direction, 
						 float DeltaTime);

	inline void RegisterOnResetCallback(const std::function<void()>& callback)
	{
		mOnResetCallback = callback;
	}
private:
    void Die();

	void FoundTreasure();
	void FoundCoin(AActor* actor);

    UFUNCTION()
    void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, 
						AActor* OtherActor,
                        UPrimitiveComponent* OtherComp, 
						int32 OtherBodyIndex,
                        bool bFromSweep, 
						const FHitResult & SweepResult);
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    float mMoveSpeed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    float mTimeBetweenDirectionSwap_s = 10;

    UPROPERTY(VisibleAnywhere)
    class UCapsuleComponent* mpCollisionComponent;

    UPROPERTY(VisibleAnywhere)
    class UStaticMeshComponent* mpMeshComponent;
private:
	bool mIsDead = true;

	EMoveDirection mCurrentDirection;

	float mTime_s = 0;

	std::function<void()> mOnResetCallback;
};