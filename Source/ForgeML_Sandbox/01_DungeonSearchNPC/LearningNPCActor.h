#pragma once

#include "BaseDungeonActor.h"

#include <functional>

#include "LearningNPCActor.generated.h"


UCLASS()
class ALearningNPCActor: public ABaseDungeonActor
{
	GENERATED_BODY()
public:
	ALearningNPCActor();
public:
	virtual void BeginPlay() override;
public:
	virtual void TickActor(float DeltaTime, 
						   enum ELevelTick TickType, 
						   FActorTickFunction& ThisTickFunction) override;
public:
	inline void SetTreasureLocation(const FVector& location)
	{
		mTreasureLocation = location;
	}

	inline void RegisterOnResetCallback(const std::function<void(ABaseDungeonActor*)>& callback)
	{
		mOnResetCallback = callback;
	}

	inline void RegisterReceiveTrainingDataCallback(const std::function<void(const TrainingInfo&)>& callback)
	{
		mTrainingDataCallback = callback;
	}

	inline void SetActionSelector(const std::function<std::tuple<EMoveDirection, float>(const std::vector<float>&)>& actionSelector)
	{
		mActionSelector = actionSelector;
	}
private:
	virtual void ResetActor(const FVector& location) override;

	virtual void OnFoundCoin() override;
	virtual void OnFoundTreasure() override;
	virtual void OnDeath() override;

	void PickNewDirection();

    void CastRayTraces(std::vector<float>* distances = nullptr, 
					   std::vector<float>* types = nullptr);

	void AddCurrentStateToTrainingData(float reward);

	float DistanceToNearestCoin();
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float mMaxTraceDistance_cm = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float mTraceHeight_cm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool mDebugTraces = false;
private:
	float mTime_s = 0;

	EMoveDirection mLastDirection = EMoveDirection::None;
	float mLastDirection_f = 0;

	std::vector<float> mRayCollisionDistances;
	std::vector<float> mRayCollisionHitTypes;

	float mLastTreasureDistance = 0;
	float mLastCoinDistance = 0;

	std::function<std::tuple<EMoveDirection, float>(const std::vector<float>&)> mActionSelector;

	std::function<void(const TrainingInfo&)> mTrainingDataCallback;
	std::function<void(ABaseDungeonActor*)> mOnResetCallback;

	FVector mTreasureLocation = FVector::ZeroVector;
};