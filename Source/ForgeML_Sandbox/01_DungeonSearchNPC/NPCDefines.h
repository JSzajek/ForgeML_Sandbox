#pragma once

#include "CoreMinimal.h"

#include <vector>

UENUM(BlueprintType)
enum class EMoveDirection : uint8 
{
	None,
	Forward,
	Backward,
	Left,
	Right,

	COUNT
};

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

const int32_t NumRayCasts = 16;
