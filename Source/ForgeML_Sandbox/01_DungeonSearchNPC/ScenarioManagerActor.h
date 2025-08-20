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


/// <summary>
/// Scenario types for the dungeon NPCs.
/// </summary>
UENUM(BlueprintType)
enum class EScenarioType : uint8 
{
	Random,
	Learning
};


/// <summary>
/// Scenario Manager Actor for managing NPCs in the dungeon.
/// </summary>
UCLASS()
class AScenarioManagerActor : public AActor
{
	GENERATED_BODY()
public:
	/// <summary>
	/// Constructor initializing a ScenarioManagerActor instance.
	/// </summary>
	AScenarioManagerActor();
public:
	/// <summary>
	/// Overridable native event for when play begins for this actor.
	/// </summary>
	virtual void BeginPlay() override;

	/// <summary>
	/// Overridable native event for when this actor is being destroyed.
	/// </summary>
	virtual void BeginDestroy() override;
public:
	/// <summary>
	/// Retrieves the number of NPCs currently managed by this actor.
	/// </summary>
	/// <returns>The number of NPCs</returns>
	UFUNCTION(BlueprintCallable)
	int32 GetNumNPCs() const { return mpNPCs.Num(); }

	/// <summary>
	/// Retrieves the current model version number/generation.
	/// </summary>
	/// <returns>The model version</returns>
	UFUNCTION(BlueprintCallable)
	int32 GetModelVersion() const;
private:
	/// <summary>
	/// Spawns NPCs based on the current scenario type.
	/// </summary>
	void SpawnNPCs();

	/// <summary>
	/// Spawns NPCs for the active scenario.
	/// </summary>
	void SpawnActiveNPC();

	/// <summary>
	/// Spawns training NPCs for the learning scenario.
	/// </summary>
	void SpawnTrainingNPCs();

	/// <summary>
	/// On ReceiveTrainingData callback to handle training data received from NPCs.
	/// </summary>
	/// <param name="newInfo">The training infor</param>
	void OnReceiveTrainingData(const TrainingInfo& newInfo);

	/// <summary>
	/// On ResetNPC callback to reset the NPC actor's position and state.
	/// </summary>
	/// <param name="actor">The actor to reset</param>
	void OnResetNPC(ABaseDungeonActor* actor);

	/// <summary>
	/// Selects the motion direction based on the inputs provided.
	/// </summary>
	/// <param name="inputs">The state input</param>
	/// <returns>The move action and float value output</returns>
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

	uint32_t mGeneration = 0;
	std::unique_ptr<TF::MLModel> mpModel = nullptr;
	std::unique_ptr<TF::FlatFloatDataBuilder> mpDataBuilder = nullptr;

	std::mutex mTrainingMutex;
	std::vector<TrainingInfo> mTrainingData {};
	FGraphEventRef mTrainingTask;
};