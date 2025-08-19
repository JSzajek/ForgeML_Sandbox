#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "NPCDefines.h"

#include "TFModelLib.h"

#include <mutex>

#include "ScenarioManagerActor.generated.h"

class ABaseDungeonActor;
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
	void SpawnNPCs();
	void SpawnActiveNPC();
	void SpawnTrainingNPCs();


	void OnReceiveTrainingData(const TrainingInfo&);

	void OnResetNPC(ABaseDungeonActor* actor);

	std::tuple<EMoveDirection, float> SelectMotion(const std::vector<float>& inputs);
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML")
	EScenarioType mCurrentScenario;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML|Training")
	bool mLiveLearning = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "ML|Training")
    int32 mNumberOfAgents = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "ML|Training")
    int32 mMaxTrainingBatches = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML|Training")
	int32 mTrainingEpochs = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML|Training")
    int32 mTrainingBatches = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML|Training")
    float mLearningRate =  0.001;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML|Training")
    float mLearningGamma =  0.95;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML")
	TSubclassOf<AActor> mpTreasureTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML")
	TSubclassOf<AActor> mpCoinTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML")
	TSubclassOf<ARandomNPCActor> mpRandomActorTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML")
	TSubclassOf<ALearningNPCActor> mpLearningActorTemplate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML")
	TArray<FVector> mSpawnPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML")
	TArray<FVector> mCoinPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ML")
	TArray<FVector> mTreasurePoints;
private:
	UPROPERTY()
	TArray<ABaseDungeonActor*> mpNPCs;

	UPROPERTY()
	AActor* mpTreasure = nullptr;

	UPROPERTY()
	TArray<AActor*> mCoins;

	FVector mTreasureLocation;


	std::unique_ptr<TF::MLModel> mpModel = nullptr;
	std::unique_ptr<TF::FlatFloatDataBuilder> mpDataBuilder = nullptr;

	std::mutex mTrainingMutex;
	std::vector<TrainingInfo> mTrainingData {};
	FGraphEventRef mTrainingTask;
};