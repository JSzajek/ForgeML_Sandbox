#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "NPCDefines.h"

#include "TFModelLib.h"

#include <functional>

#include "LearningNPCActor.generated.h"

struct TrainingStateInfo
{
	std::vector<float> mRayCollisionDistances;
	std::vector<float> mRayCollisionHitTypes;
	float mTreasureDistance = 0;
};

struct TrainingInfo
{
	EMoveDirection mDirection = EMoveDirection::None;
	float mDirection_f = 0.f;

	TrainingStateInfo mState;

	float mReward = 0;
};


UCLASS()
class ALearningNPCActor: public AActor
{
	GENERATED_BODY()
public:
	ALearningNPCActor();
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

	inline void SetTreasureLocation(const FVector& location)
	{
		mTreasureLocation = location;
	}

	inline void RegisterOnResetCallback(const std::function<void()>& callback)
	{
		mOnResetCallback = callback;
	}
private:
	void PickNewDirection();

    void CastRayTraces(std::vector<float>* distances = nullptr, 
					   std::vector<float>* types = nullptr);

	void AddCurrentStateToTrainingData(float reward);

	void TrainBatches();
	
	void Die();

	void FoundTreasure();
	void FoundCoin(AActor* actor);

	float DistanceToNearestCoin();

    UFUNCTION()
    void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, 
						AActor* OtherActor,
                        UPrimitiveComponent* OtherComp, 
						int32 OtherBodyIndex,
                        bool bFromSweep, 
						const FHitResult & SweepResult);
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool mTraining = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    float mMoveSpeed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    float mTimeBetweenDirectionSwap_s = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float mMaxTraceDistance_cm = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float mTraceHeight_cm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 mMaxTrainingBatches = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 mTrainingEpochs = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 mTrainingBatches = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool mDebugTraces = false;

    UPROPERTY(VisibleAnywhere)
    class UCapsuleComponent* mpCollisionComponent;

    UPROPERTY(VisibleAnywhere)
    class UStaticMeshComponent* mpMeshComponent;
private:
	bool mIsRunning = true;
	bool mIsDead = true;

	float mTime_s = 0;

	std::unique_ptr<TF::MLModel> mpModel = nullptr;
	std::unique_ptr<TF::FlatFloatDataBuilder> mpDataBuilder = nullptr;

	std::vector<float> mRayCollisionDistances;
	std::vector<float> mRayCollisionHitTypes;
	EMoveDirection mLastDirection;
	float mLastDirection_f = 0;
	float mLastTreasureDistance = 0;
	float mLastCoinDistance = 0;

	std::vector<TrainingInfo> mTrainingData;

	FVector mTreasureLocation = FVector::ZeroVector;

	std::function<void()> mOnResetCallback;
};