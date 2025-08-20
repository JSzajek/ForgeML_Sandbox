#pragma once

#include "BaseDungeonActor.h"

#include <functional>

#include "LearningNPCActor.generated.h"


/// <summary>
/// A Learning NPC Actor that navigates a 
/// dungeon to find coins and treasure.
/// </summary>
UCLASS()
class ALearningNPCActor: public ABaseDungeonActor
{
	GENERATED_BODY()
public:
	/// <summary>
	/// Constructor initializing a ALearningNPCActor instance.
	/// </summary>
	ALearningNPCActor();
public:
	/// <summary>
	/// Overridable native event for when play begins for this actor.
	/// </summary>
	virtual void BeginPlay() override;
public:
	/// <summary>
	/// Dispatches the once-per frame Tick() function for this actor.
	/// </summary>
	/// <param name="DeltaTime">The time slice of this tick</param>
	/// <param name="TickType">The type of tick that is happening</param>
	/// <param name="ThisTickFunction">The tick function that is firing, useful for getting the completion handle</param>
	virtual void TickActor(float DeltaTime, 
						   enum ELevelTick TickType, 
						   FActorTickFunction& ThisTickFunction) override;
public:
	/// <summary>
	/// Sets the treasure location for this actor.
	/// </summary>
	/// <param name="location">The new treasure location</param>
	inline void SetTreasureLocation(const FVector& location)
	{
		mTreasureLocation = location;
	}

	/// <summary>
	/// Registers a callback to be invoked when the actor is reset.
	/// </summary>
	/// <param name="callback">The callback</param>
	inline void RegisterOnResetCallback(const std::function<void(ABaseDungeonActor*)>& callback)
	{
		mOnResetCallback = callback;
	}

	/// <summary>
	/// Registers a callback to receive training data from the actor.
	/// </summary>
	/// <param name="callback">The callback</param>
	inline void RegisterReceiveTrainingDataCallback(const std::function<void(const TrainingInfo&)>& callback)
	{
		mTrainingDataCallback = callback;
	}

	/// <summary>
	/// Registers a function to select the action based on the current state.
	/// </summary>
	/// <param name="actionSelector">The action selector callback</param>
	inline void SetActionSelector(const std::function<std::tuple<EMoveDirection, float>(const std::vector<float>&)>& actionSelector)
	{
		mActionSelector = actionSelector;
	}
private:
	/// <summary>
	/// Resets the actor to a specific location.
	/// </summary>
	/// <param name="location">The new location to set</param>
	virtual void ResetActor(const FVector& location) override;

	/// <summary>
	/// Overridable native event called when the actor finds a coin.
	/// </summary>
	virtual void OnFoundCoin() override;

	/// <summary>
	/// Overridable native event called when the actor finds the treasure.
	/// </summary>
	virtual void OnFoundTreasure() override;

	/// <summary>
	/// Overridable native event called when the actor dies.
	/// </summary>
	virtual void OnDeath() override;

	/// <summary>
	/// Picks a new direction for the actor to move in based on 
	/// the current state and action selector.
	/// </summary>
	void PickNewDirection();

    /// <summary>
	/// Casts ray traces in the current direction to detect obstacles, coins, and treasure.
    /// </summary>
    /// <param name="distances">The output distances</param>
    /// <param name="types">The output hit types</param>
    void CastRayTraces(std::vector<float>* distances = nullptr, 
					   std::vector<float>* types = nullptr);

	/// <summary>
	/// Adds the current state of the actor to the training data.
	/// </summary>
	/// <param name="reward">The reward for the current state</param>
	void AddCurrentStateToTrainingData(float reward);

	/// <summary>
	/// Finds the closest distance coin to the actor's current location.
	/// </summary>
	/// <returns>The distance</returns>
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