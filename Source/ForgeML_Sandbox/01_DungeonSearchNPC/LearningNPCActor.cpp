#include "LearningNPCActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"



const int32_t NumRayCasts = 16;

ALearningNPCActor::ALearningNPCActor()
{
	PrimaryActorTick.bCanEverTick = true;

	mIsDead = false;

	// Collision capsule
	mpCollisionComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionComp"));
	RootComponent = mpCollisionComponent;
	mpCollisionComponent->InitCapsuleSize(20.f, 20.f);
	mpCollisionComponent->SetCollisionProfileName("Pawn");
	mpCollisionComponent->SetGenerateOverlapEvents(true);

	// Mesh
	mpMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	mpMeshComponent->SetupAttachment(RootComponent);

	mpDataBuilder = std::make_unique<TF::FlatFloatDataBuilder>((NumRayCasts * 2) + 1,
															   std::vector<int64_t>{ (NumRayCasts * 2) + 1 });

	mpModel = nullptr;
	mTime_s = 0;
}

void ALearningNPCActor::BeginPlay()
{
	Super::BeginPlay();

	if (mpCollisionComponent)
		mpCollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ALearningNPCActor::OnBeginOverlap);

	mpModel = std::make_unique<TF::MLModel>("01_DungeonNavigator");

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

void ALearningNPCActor::BeginDestroy()
{
	Super::BeginDestroy();

	if (mpCollisionComponent)
		mpCollisionComponent->OnComponentBeginOverlap.RemoveDynamic(this, &ALearningNPCActor::OnBeginOverlap);
}

void ALearningNPCActor::TickActor(float DeltaTime, 
								  enum ELevelTick TickType, 
								  FActorTickFunction& ThisTickFunction)
{
	if (!mIsRunning)
		return;

	if (mIsDead)
		return;

	if (mTime_s < mTimeBetweenDirectionSwap_s)
	{
		MoveInDirection(mLastDirection, DeltaTime);
		mTime_s += DeltaTime;

		CastRayTraces();
	}
	else
	{
		mTime_s = 0;

		if (!mIsDead)
		{
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

			if (mTraining)
			{
				AddCurrentStateToTrainingData(reward);

				if (mTrainingData.size() >= mMaxTrainingBatches)
				{
					TrainBatches();

					mTrainingData.clear();

					if (mOnResetCallback)
						mOnResetCallback();

					return;
				}
			}

			PickNewDirection();
		}
	}
}

void ALearningNPCActor::MoveInDirection(EMoveDirection direction,
										float deltaTime)
{
	FVector MovementVector;
	switch (mLastDirection)
	{
	case EMoveDirection::Forward:
		MovementVector = GetActorForwardVector();
		break;
	case EMoveDirection::Backward:
		MovementVector = -GetActorForwardVector();
		break;
	case EMoveDirection::Left:
		MovementVector = -GetActorRightVector();
		break;
	case EMoveDirection::Right:
		MovementVector = GetActorRightVector();
		break;
	default:
		return; // No movement for None
	}

	// Attempt move
	FHitResult Hit;
	AddActorWorldOffset(MovementVector * mMoveSpeed * deltaTime, true, &Hit);
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

	float randChance = mTraining ? FMath::FRandRange(0.0f, 1.0f) : 1.0f;
	float action_f = 0;

	EMoveDirection action = EMoveDirection::None;
	if (randChance <= 0.3)
	{
		// Random Exploration
		action = static_cast<EMoveDirection>(FMath::RandRange((int)EMoveDirection::None, (int)EMoveDirection::COUNT - 1));
		action_f = static_cast<float>(action);

		UE_LOG(LogTemp, Display, TEXT("Random Move Direction: %d"), action);
	}
	else
	{
		// Use Model to Decide Action
		if (mpModel && !mIsDead)
		{
			// Add Ray Collision Distances
			std::vector<float> stateInfo;
			for (uint32_t i = 0; i < NumRayCasts; ++i)
			{
				stateInfo.emplace_back(collisionDistances[i]);
				stateInfo.emplace_back(collisionTypes[i]);
			}

			stateInfo.emplace_back(mLastTreasureDistance);

			mpDataBuilder->AddInputTensor("state", stateInfo);


			TF::LabeledTensor inputs;
			if (mpDataBuilder->CreateTensor(inputs))
			{
				TF::LabeledTensor outputs;
				if (mpModel->Run(inputs, outputs))
				{
					cppflow::tensor actionTensor = outputs["action"];

					const std::vector<float> actionData = actionTensor.get_data<float>();
					if (actionData.size() == 1)
					{
						action_f = actionData[0];

						int actionIndex = static_cast<int>(FMath::RoundToInt(action_f));
						actionIndex = FMath::Clamp(actionIndex, (int)EMoveDirection::None, static_cast<int>(EMoveDirection::COUNT) - 1);

						UE_LOG(LogTemp, Display, TEXT("Prediction [%f] Move Direction: %d"), action_f, actionIndex);

						action = static_cast<EMoveDirection>(actionIndex);
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("Invalid Action Output!"));
						return;
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to Run Model!"));
					return;
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to Create Input!"));
				return;
			}
		}
	}

	mLastDirection = action;
	mLastDirection_f = action_f;
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
	if (!mpModel)
		return;

	if (mRayCollisionDistances.size() != NumRayCasts)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Number of Ray Inputs"));
		return;
	}

	TrainingInfo info;

	info.mDirection = mLastDirection;
	info.mDirection_f = mLastDirection_f;
	info.mReward = reward;

	info.mState.mRayCollisionDistances = mRayCollisionDistances;
	info.mState.mRayCollisionHitTypes = mRayCollisionHitTypes;
	info.mState.mTreasureDistance = mLastTreasureDistance;

	mTrainingData.emplace_back(info);
}

void ALearningNPCActor::TrainBatches()
{
	UE_LOG(LogTemp, Display, TEXT("NPC Starting Training..."));

	if (mpModel)
	{
		for (const TrainingInfo& info : mTrainingData)
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
								 mTrainingBatches))
		{
			UE_LOG(LogTemp, Warning, TEXT("NPC Training Failed!"));
		}
	}

	UE_LOG(LogTemp, Display, TEXT("NPC Finished Training..."));
}

void ALearningNPCActor::Die()
{
	UE_LOG(LogTemp, Display, TEXT("NPC has Died!"));

	mIsDead = true;

	AddCurrentStateToTrainingData(-1000);
	TrainBatches();

	if (mOnResetCallback)
		mOnResetCallback();
}

void ALearningNPCActor::FoundTreasure()
{
	UE_LOG(LogTemp, Display, TEXT("NPC Found Treasure!"));

	AddCurrentStateToTrainingData(1000);
	TrainBatches();

	if (mOnResetCallback)
		mOnResetCallback();
}

void ALearningNPCActor::FoundCoin(AActor* actor)
{
	UE_LOG(LogTemp, Display, TEXT("NPC Found Coin!"));

	AddCurrentStateToTrainingData(100);

	actor->Destroy();
}

float ALearningNPCActor::DistanceToNearestCoin()
{
	TArray<AActor*> foundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), "Coin", foundActors);
	float nearestDistance = TNumericLimits<float>::Max();
	for (AActor* actor : foundActors)
	{
		float dist = FVector::Distance(actor->GetActorLocation(), GetActorLocation());
		if (dist < nearestDistance)
			nearestDistance = dist;
	}
	return nearestDistance;
}

void ALearningNPCActor::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, 
									   AActor* OtherActor, 
									   UPrimitiveComponent* OtherComp, 
									   int32 OtherBodyIndex, 
									   bool bFromSweep, 
									   const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this)
	{
		// Actor-level tag check
		if (OtherActor->ActorHasTag("Hazard"))
			Die();

		if (OtherActor->ActorHasTag("Treasure"))
			FoundTreasure();

		if (OtherActor->ActorHasTag("Coin"))
			FoundCoin(OtherActor);

		// Component-level tag check
		if (OtherComp)
		{
			if (OtherComp->ComponentHasTag("Hazard"))
				Die();

			if (OtherComp->ComponentHasTag("Treasure"))
				FoundTreasure();

			if (OtherComp->ComponentHasTag("Coin"))
				FoundCoin(OtherComp->GetOwner());
		}
	}
}