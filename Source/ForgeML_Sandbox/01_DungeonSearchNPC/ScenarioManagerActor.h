#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "ScenarioManagerActor.generated.h"

class ARandomNPCActor;
class ALearningNPCActor;


UENUM(BlueprintType)
enum class EScenarioType : uint8 
{
	Random,
	Learning
};

UCLASS()
class AScenarioManagerActor : public AActor
{
	GENERATED_BODY()
public:
	AScenarioManagerActor();
public:
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
private:
	void SpawnNPC();

	void OnNPCReset();
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EScenarioType mCurrentScenario;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool mLiveLearning = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> mpTreasureTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> mpCoinTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ARandomNPCActor> mpRandomActorTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ALearningNPCActor> mpLearningActorTemplate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> mSpawnPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> mCoinPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FVector> mTreasurePoints;
private:
	UPROPERTY()
	AActor* mpNPC = nullptr;

	UPROPERTY()
	AActor* mpTreasure = nullptr;

	UPROPERTY()
	TArray<AActor*> mCoins;
};