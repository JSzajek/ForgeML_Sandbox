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

	mpDataBuilder = std::make_unique<TF::FlatFloatDataBuilder>((NumRayCasts * 2) + 1,
															   std::vector<int64_t>{ (NumRayCasts * 2) + 1 });

	mpModel = std::make_unique<TF::MLModel>("01_DungeonNavigator");

	mGeneration = mpModel->GetModelVersion();

	if (mpModel->DoesModelExists())
	{
		if (!mpModel->LoadIfExists())
			mpModel = nullptr;
	}
	else
	{
		mpModel->AddInput("state",
						  TF::DataType::Float32, 
						  { -1, (NumRayCasts * 2) + 1 });

		mpModel->AddOutput("action");

		mpModel->AddLayer(TF::LayerType::Flatten,
		{
			{ "input_name", "state" },
			{ "output_name", "flat_input" }
		});

		mpModel->AddLayer(TF::LayerType::Dense,
		{
			{ "input_name", "flat_input" },
			{ "units", 64 },
			{ "output_name", "dense_1" },
			{ "activation", "relu" },
		});

		mpModel->AddLayer(TF::LayerType::Dense,
		{
			{ "input_name", "dense_1" },
			{ "units", 256 },
			{ "output_name", "dense_2" },
			{ "activation", "relu" },
		});

		mpModel->AddLayer(TF::LayerType::Dense,
		{
			{ "input_name", "dense_2" },
			{ "units", 128 },
			{ "output_name", "dense_3" },
			{ "activation", "relu" },
		});

		mpModel->AddLayer(TF::LayerType::Dense,
		{
			{ "input_name", "dense_3" },
			{ "units", 1 },
			{ "activation", "linear" },
			{ "output_name", "action" },
		});

		if (!mpModel->CreateModel())
		{
			UE_LOG(LogTemp, Display, TEXT("Failed To Create Model!"));
			mpModel = nullptr;
		}
	}

	SpawnNPCs();
}

void AScenarioManagerActor::BeginDestroy()
{
	Super::BeginDestroy();

	if (mTrainingTask)
		mTrainingTask->Wait();
}

int32 AScenarioManagerActor::GetModelVersion() const
{
	if (!mpModel)
	{
		UE_LOG(LogTemp, Warning, TEXT("Model is not initialized!"));
		return 0;
	}

	return mpModel->GetModelVersion();
}

void AScenarioManagerActor::SpawnNPCs()
{
	if (mTreasurePoints.IsEmpty())
		return;

	// Spawn the Treasure Point
	mTreasureLocation = mTreasurePoints[FMath::RandRange(0, mTreasurePoints.Num() - 1)];

	mpTreasure = GetWorld()->SpawnActor<AActor>(mpTreasureTemplate, mTreasureLocation, FRotator::ZeroRotator);
	mpTreasure->SetActorScale3D(FVector(10));

	// Spawn Coins
	for (const FVector& point : mCoinPoints)
	{
		AActor* coin = GetWorld()->SpawnActor<AActor>(mpCoinTemplate, point, FRotator::ZeroRotator);
		coin->SetActorScale3D(FVector(10));

		mCoins.Add(coin);
	}

	if (mLiveLearning)
		SpawnTrainingNPCs();
	else
		SpawnActiveNPC();
}

void AScenarioManagerActor::SpawnActiveNPC()
{
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

			actor->RegisterOnResetCallback(std::bind(&AScenarioManagerActor::OnResetNPC, this, std::placeholders::_1));

			mpNPCs.Add(actor);
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
			actor->SetTreasureLocation(mpTreasure->GetActorLocation());

			actor->RegisterOnResetCallback(std::bind(&AScenarioManagerActor::OnResetNPC, this, std::placeholders::_1));
			actor->RegisterReceiveTrainingDataCallback(std::bind(&AScenarioManagerActor::OnReceiveTrainingData, this, std::placeholders::_1));
			actor->SetActionSelector(std::bind(&AScenarioManagerActor::SelectMotion, this, std::placeholders::_1));

			mpNPCs.Add(actor);
		}
	}
}

void AScenarioManagerActor::SpawnTrainingNPCs()
{
	if (mCurrentScenario != EScenarioType::Learning)
		return;

	for (int32_t i = 0; i < mNumberOfAgents; ++i)
	{
		FVector SpawnLocation = mSpawnPoints[FMath::RandRange(0, mSpawnPoints.Num() - 1)];
		ALearningNPCActor* npc = GetWorld()->SpawnActor<ALearningNPCActor>(mpLearningActorTemplate, SpawnLocation, FRotator::ZeroRotator);
		npc->SetActorScale3D(FVector(10));
		npc->SetTreasureLocation(mpTreasure->GetActorLocation());

		npc->RegisterOnResetCallback(std::bind(&AScenarioManagerActor::OnResetNPC, this, std::placeholders::_1));
		npc->RegisterReceiveTrainingDataCallback(std::bind(&AScenarioManagerActor::OnReceiveTrainingData, this, std::placeholders::_1));
		npc->SetActionSelector(std::bind(&AScenarioManagerActor::SelectMotion, this, std::placeholders::_1));

		mpNPCs.Add(npc);
	}
}

void AScenarioManagerActor::OnReceiveTrainingData(const TrainingInfo& newInfo)
{
	const std::scoped_lock lock(mTrainingMutex);

	if (mCurrentScenario != EScenarioType::Learning)
		return;

	mTrainingData.emplace_back(newInfo);

	if (!mpModel)
	{
		UE_LOG(LogTemp, Warning, TEXT("Model is not initialized!"));
		return;
	}

	if (mTrainingData.size() >= mMaxTrainingBatches)
	{
		std::vector<TrainingInfo> localTrainingData = std::move(mTrainingData);

		if (mTrainingTask)
			mTrainingTask->Wait();

		mTrainingTask = FFunctionGraphTask::CreateAndDispatchWhenReady([localTrainingData, this]()
		{
			UE_LOG(LogTemp, Display, TEXT("NPC Starting Training..."));

			for (const TrainingInfo& info : localTrainingData)
			{
				nlohmann::json stateInfo;
				for (uint32_t i = 0; i < NumRayCasts; ++i)
				{
					stateInfo.emplace_back(info.mState.mRayCollisionDistances[i]);
					stateInfo.emplace_back(info.mState.mRayCollisionHitTypes[i]);
				}

				stateInfo.push_back(info.mState.mTreasureDistance);

				nlohmann::json actionValue;
				actionValue.push_back(info.mDirection_f);

				mpModel->AddRewardData(stateInfo, actionValue, info.mReward);
			}

			if (!mpModel->TrainModel(mTrainingEpochs,
									 mTrainingBatches, 
									 mLearningRate,
									 mLearningGamma))
			{
				UE_LOG(LogTemp, Warning, TEXT("NPC Training Failed!"));
			}

			UE_LOG(LogTemp, Display, TEXT("NPC Finished Training..."));

		}, TStatId(), nullptr, ENamedThreads::AnyBackgroundThreadNormalTask);


		for (ABaseDungeonActor* actor : mpNPCs)
			OnResetNPC(actor);
	}
}

void AScenarioManagerActor::OnResetNPC(ABaseDungeonActor* actor)
{
	if (!actor)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnResetNPC called with null actor!"));
		return;
	}

	int32 SpawnIndex = FMath::RandRange(0, mSpawnPoints.Num() - 1);
	FVector SpawnLocation = mSpawnPoints[SpawnIndex];
	actor->ResetActor(SpawnLocation);
}

std::tuple<EMoveDirection, float> AScenarioManagerActor::SelectMotion(const std::vector<float>& inputs)
{
	float randChance = mLiveLearning ? FMath::FRandRange(0.0f, 1.0f) : 1.0f;

	EMoveDirection action = EMoveDirection::None;
	float action_f = 0;

	if (randChance <= 0.3)
	{
		// Random Exploration
		action = static_cast<EMoveDirection>(FMath::RandRange((int)EMoveDirection::None, (int)EMoveDirection::COUNT - 1));
		action_f = static_cast<float>(action);
	}
	else
	{
		// Use Model to Decide Action
		if (mpModel)
		{
			if (!mpDataBuilder)
			{
				UE_LOG(LogTemp, Warning, TEXT("Data Builder is not initialized!"));
				return { EMoveDirection::None, 0.0f };
			}

			mpDataBuilder->AddInputTensor("state", inputs);


			TF::LabeledTensor labeled_inputs;
			if (mpDataBuilder->CreateTensor(labeled_inputs))
			{
				TF::LabeledTensor outputs;
				if (mpModel->Run(labeled_inputs, outputs))
				{
					cppflow::tensor actionTensor = outputs["action"];

					const std::vector<float> actionData = actionTensor.get_data<float>();
					if (actionData.size() == 1)
					{
						action_f = actionData[0];

						int actionIndex = static_cast<int>(FMath::RoundToInt(action_f));
						actionIndex = FMath::Clamp(actionIndex, (int)EMoveDirection::None, static_cast<int>(EMoveDirection::COUNT) - 1);

						action = static_cast<EMoveDirection>(actionIndex);
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("Invalid Action Output!"));
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to Run Model!"));
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to Create Input!"));
			}
		}
	}

	return { action, action_f };
}