#pragma once

#include "BaseDungeonActor.h"

#include <functional>

#include "RandomNPCActor.generated.h"

UCLASS()
class ARandomNPCActor : public ABaseDungeonActor
{
	GENERATED_BODY()
public:
	ARandomNPCActor();
public:
	virtual void TickActor(float DeltaTime, 
						   enum ELevelTick TickType, 
						   FActorTickFunction& ThisTickFunction) override;
public:
	inline void RegisterOnResetCallback(const std::function<void(ABaseDungeonActor*)>& callback)
	{
		mOnResetCallback = callback;
	}
private:
	virtual void ResetActor(const FVector& location) override;

	virtual void OnFoundCoin() override;
	virtual void OnFoundTreasure() override;
	virtual void OnDeath() override;
private:
	EMoveDirection mCurrentDirection;

	float mTime_s = 0;

	std::function<void(ABaseDungeonActor*)> mOnResetCallback;
};