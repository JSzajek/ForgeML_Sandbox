#include "ScenarioManagerActor.h"

#include "Engine/World.h"

#include "RandomNPCActor.h"
#include "LearningNPCActor.h"

AScenarioManagerActor::AScenarioManagerActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AScenarioManagerActor::BeginPlay()
{
	Super::BeginPlay();

	SpawnNPC();
}

void AScenarioManagerActor::BeginDestroy()
{
	Super::BeginDestroy();
}

void AScenarioManagerActor::SpawnNPC()
{
	// Spawn the Treasure Point
	int32 TreasureSpawnIndex = FMath::RandRange(0, mTreasurePoints.Num() - 1);
	mpTreasure = GetWorld()->SpawnActor<AActor>(mpTreasureTemplate, mTreasurePoints[TreasureSpawnIndex], FRotator::ZeroRotator);
	mpTreasure->SetActorScale3D(FVector(10));


	for (const FVector& point : mCoinPoints)
	{
		AActor* coin = GetWorld()->SpawnActor<AActor>(mpCoinTemplate, point, FRotator::ZeroRotator);
		coin->SetActorScale3D(FVector(10));

		mCoins.Add(coin);
	}

	if (mCurrentScenario == EScenarioType::Random)
	{
		if (!mpRandomActorTemplate)
			return;

		if (mSpawnPoints.Num() > 0)
		{
			int32 SpawnIndex = FMath::RandRange(0, mSpawnPoints.Num() - 1);
			FVector SpawnLocation = mSpawnPoints[SpawnIndex];
			
			// Assuming you have a class for the NPC actor
			ARandomNPCActor* actor = GetWorld()->SpawnActor<ARandomNPCActor>(mpRandomActorTemplate, SpawnLocation, FRotator::ZeroRotator);
			actor->SetActorScale3D(FVector(10));

			actor->RegisterOnResetCallback(std::bind(&AScenarioManagerActor::OnNPCReset, this));
			mpNPC = actor;
		}
	}
	else if (mCurrentScenario == EScenarioType::Learning)
	{
		if (!mpLearningActorTemplate)
			return;

		if (mSpawnPoints.Num() > 0)
		{
			int32 SpawnIndex = FMath::RandRange(0, mSpawnPoints.Num() - 1);
			FVector SpawnLocation = mSpawnPoints[SpawnIndex];

			// Assuming you have a class for the NPC actor
			ALearningNPCActor* actor = GetWorld()->SpawnActor<ALearningNPCActor>(mpLearningActorTemplate, SpawnLocation, FRotator::ZeroRotator);
			actor->SetActorScale3D(FVector(10));
			actor->mTraining = mLiveLearning;

			actor->SetTreasureLocation(mpTreasure->GetActorLocation());
			actor->RegisterOnResetCallback(std::bind(&AScenarioManagerActor::OnNPCReset, this));
			mpNPC = actor;
		}
	}
}

void AScenarioManagerActor::OnNPCReset()
{
	if (mpNPC)
	{
		mpNPC->Destroy();
		mpNPC = nullptr;
	}

	if (mpTreasure)
	{
		mpTreasure->Destroy();
		mpTreasure = nullptr;
	}

	for (AActor* actor : mCoins)
	{
		if (actor)
		{
			actor->Destroy();
		}
	}
	mCoins = {};

	SpawnNPC();
}