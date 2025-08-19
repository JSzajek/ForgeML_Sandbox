#include "RandomNPCActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Engine/World.h"

ARandomNPCActor::ARandomNPCActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ARandomNPCActor::TickActor(float DeltaTime, 
								enum ELevelTick TickType, 
								FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	if (mTime_s < mTimeBetweenDirectionSwap_s)
	{
		MoveInDirection(mCurrentDirection, DeltaTime);
		mTime_s += DeltaTime;
	}
	else
	{
		mCurrentDirection = static_cast<EMoveDirection>(FMath::RandRange((int)EMoveDirection::None, (int)EMoveDirection::COUNT));
		mTime_s = 0;
	}
}

void ARandomNPCActor::OnDeath()
{
	//UE_LOG(LogTemp, Display, TEXT("NPC has Died!"));

	if (mOnResetCallback)
		mOnResetCallback(this);
}

void ARandomNPCActor::ResetActor(const FVector& location)
{
	ABaseDungeonActor::ResetActor(location);

	mCurrentDirection = EMoveDirection::None;
}

void ARandomNPCActor::OnFoundCoin()
{
	//UE_LOG(LogTemp, Display, TEXT("NPC Found Coin!"));
}

void ARandomNPCActor::OnFoundTreasure()
{
	//UE_LOG(LogTemp, Display, TEXT("NPC Found Treasure!"));

	if (mOnResetCallback)
		mOnResetCallback(this);
}