#pragma once

#include "BaseDungeonActor.h"

#include <functional>

#include "RandomNPCActor.generated.h"

/// <summary>
///  A Random NPC Actor that randomly moves 
///  in a dungeon to find coins and treasure.
/// </summary>
UCLASS()
class ARandomNPCActor : public ABaseDungeonActor
{
	GENERATED_BODY()
public:
	/// <summary>
	/// Constructor initializing a ARandomNPCActor instance.
	/// </summary>
	ARandomNPCActor();
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
	/// Registers a callback to be invoked when the actor is reset.
	/// </summary>
	/// <param name="callback">The callback</param>
	inline void RegisterOnResetCallback(const std::function<void(ABaseDungeonActor*)>& callback)
	{
		mOnResetCallback = callback;
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
private:
	EMoveDirection mCurrentDirection;

	float mTime_s = 0;

	std::function<void(ABaseDungeonActor*)> mOnResetCallback;
};