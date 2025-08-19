#include "LearningNPCActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include "TFModelLib.h"

ALearningNPCActor::ALearningNPCActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ALearningNPCActor::BeginPlay()
{
	Super::BeginPlay();

	// Cache Distance to Treasure.
	mLastTreasureDistance = FVector::Distance(mTreasureLocation, GetActorLocation());
	mLastCoinDistance = DistanceToNearestCoin();

	// Get Current Collision Query Distances.
	std::vector<float> collisionDistances;
	std::vector<float> collisionTypes;
	CastRayTraces(&collisionDistances, &collisionTypes);

	mRayCollisionDistances = collisionDistances;
	mRayCollisionHitTypes = collisionTypes;
}

void ALearningNPCActor::TickActor(float DeltaTime, 
								  enum ELevelTick TickType, 
								  FActorTickFunction& ThisTickFunction)
{
	if (mTime_s < mTimeBetweenDirectionSwap_s)
	{
		MoveInDirection(mLastDirection, DeltaTime);
		mTime_s += DeltaTime;

		CastRayTraces();
	}
	else
	{
		mTime_s = 0;

		// Initial small negative reward for each action to encourage efficiency
		float reward = -0.01f;

		bool canSeeTreasure = false;
		bool canSeeCoin = false;
		for (float hit : mRayCollisionHitTypes)
		{
			if (hit == 4.0f) // Assuming 4.0f is the type for Treasure
			{
				canSeeTreasure = true;
				break;
			}

			if (hit == 3.0f)
			{
				canSeeCoin = true;
			}
		}

		// Small positive reward for keeping treasure in view
		if (canSeeTreasure)
			reward += 0.2f;

		if (canSeeCoin)
			reward += 0.1f;

		float distToTreasure = FVector::Distance(mTreasureLocation, GetActorLocation());
		float delta = mLastTreasureDistance - distToTreasure;

		reward += delta * 0.05f;

		if (!canSeeTreasure && delta > 0)
			reward -= 0.1f; // penalize blind progress


		// Distance shaping for nearest coin
		float distToNearestCoin = DistanceToNearestCoin();
		float coinDelta = mLastCoinDistance - distToNearestCoin;

		if (coinDelta > 0)
			reward += coinDelta * 0.05f; // small shaping reward

		mLastCoinDistance = distToNearestCoin;

		AddCurrentStateToTrainingData(reward);

		PickNewDirection();
	}
}

void ALearningNPCActor::ResetActor(const FVector& location)
{
	ABaseDungeonActor::ResetActor(location);

	mLastDirection = EMoveDirection::None;
}

void ALearningNPCActor::OnFoundCoin()
{
	//UE_LOG(LogTemp, Display, TEXT("NPC Found Coin!"));

	AddCurrentStateToTrainingData(2.5f);
}

void ALearningNPCActor::OnFoundTreasure()
{
	//UE_LOG(LogTemp, Display, TEXT("NPC Found Treasure!"));

	AddCurrentStateToTrainingData(100);

	if (mOnResetCallback)
		mOnResetCallback(this);
}

void ALearningNPCActor::OnDeath()
{
	//UE_LOG(LogTemp, Display, TEXT("NPC has Died!"));

	AddCurrentStateToTrainingData(-100);

	if (mOnResetCallback)
		mOnResetCallback(this);
}

void ALearningNPCActor::PickNewDirection()
{
	FVector centerPosition = GetActorLocation();

	// Cache Distance to Treasure.
	mLastTreasureDistance = FVector::Distance(mTreasureLocation, centerPosition);

	// Get Current Collision Query Distances.
	std::vector<float> collisionDistances;
	std::vector<float> collisionTypes;
	CastRayTraces(&collisionDistances, &collisionTypes);

	// Cache the distances for training.
	mRayCollisionDistances = collisionDistances;
	mRayCollisionHitTypes = collisionTypes;

	// Add Ray Collision Distances
	std::vector<float> stateInfo;
	for (uint32_t i = 0; i < NumRayCasts; ++i)
	{
		stateInfo.emplace_back(collisionDistances[i]);
		stateInfo.emplace_back(collisionTypes[i]);
	}

	stateInfo.emplace_back(mLastTreasureDistance);

	const auto [dir, dir_f] = mActionSelector(stateInfo);

	mLastDirection = dir;
	mLastDirection_f = dir_f;
}

void ALearningNPCActor::CastRayTraces(std::vector<float>* distances, 
									  std::vector<float>* types)
{
	FVector centerPosition = GetActorLocation();
	centerPosition.Z = mTraceHeight_cm;

	for (int32 i = 0; i < NumRayCasts; i++)
	{
		float AngleDeg = (360.f / NumRayCasts) * i;
		float AngleRad = FMath::DegreesToRadians(AngleDeg);

		// Forward vector in XY plane
		FVector Direction = FVector(FMath::Cos(AngleRad), FMath::Sin(AngleRad), 0.f);

		FVector Start = centerPosition;
		FVector End = Start + Direction * mMaxTraceDistance_cm;

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);  // don't hit self

		for (AActor* coin : mVisitedCoins)
		{
			Params.AddIgnoredActor(coin);  // don't hit already collected coins
		}

		bool isHit = GetWorld()->LineTraceSingleByChannel(Hit,
														  Start,
														  End,
														  ECC_WorldStatic,
														  Params);

		float distance = isHit ? Hit.Distance : mMaxTraceDistance_cm;

		// Normalize to [0,1]
		float normalizedDistance = distance / mMaxTraceDistance_cm;

		float type = isHit ? 1.0f : 0.0f;

		if (isHit && Hit.Component.IsValid())
		{
			if (Hit.Component->ComponentHasTag("Hazard"))
				type = 2.0f;

			if (Hit.Component->ComponentHasTag("Coin"))
				type = 3.0f;

			if (Hit.Component->ComponentHasTag("Treasure"))
				type = 4.0f;
		}

		//UE_LOG(LogTemp, Log, TEXT("Ray %d: %.2f (normalized %.2f)"), i, distance, normalizedDistance);

		if (mDebugTraces)
		{
			if (isHit)
			{
				DrawDebugLine(GetWorld(), 
							  centerPosition, 
							  Hit.ImpactPoint,
							  FColor::Red,
							  false,
							  0.1f,
							  0, 5);
			}
			else
			{
				DrawDebugLine(GetWorld(), 
							  centerPosition, 
							  End,
							  FColor::Green,
							  false,
							  0.1f,
							  0, 5);
			}
		}

		if (distances)
			distances->emplace_back(normalizedDistance);

		if (types)
			types->emplace_back(type);
	}
}

void ALearningNPCActor::AddCurrentStateToTrainingData(float reward)
{
	if (mTrainingDataCallback)
	{
		TrainingInfo info;
		info.mDirection = mLastDirection;
		info.mDirection_f = mLastDirection_f;
		info.mReward = reward;
		info.mState.mRayCollisionDistances = mRayCollisionDistances;
		info.mState.mRayCollisionHitTypes = mRayCollisionHitTypes;
		info.mState.mTreasureDistance = mLastTreasureDistance;

		mTrainingDataCallback(info);
	}
}

float ALearningNPCActor::DistanceToNearestCoin()
{
	TArray<AActor*> foundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Coin", foundActors);
	float nearestDistance = TNumericLimits<float>::Max();
	for (AActor* actor : foundActors)
	{
		// Prevent counting already visited coins.
		if (mVisitedCoins.contains(actor))
			continue;

		float dist = FVector::Distance(actor->GetActorLocation(), GetActorLocation());
		if (dist < nearestDistance)
			nearestDistance = dist;
	}
	return nearestDistance;
}