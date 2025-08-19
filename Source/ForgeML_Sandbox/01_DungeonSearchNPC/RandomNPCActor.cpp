#include "RandomNPCActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"

#include "Engine/World.h"

ARandomNPCActor::ARandomNPCActor()
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
}

void ARandomNPCActor::BeginPlay()
{
	Super::BeginPlay();

	if (mpCollisionComponent)
		mpCollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ARandomNPCActor::OnBeginOverlap);
}

void ARandomNPCActor::BeginDestroy()
{
	Super::BeginDestroy();

	if (mpCollisionComponent)
		mpCollisionComponent->OnComponentBeginOverlap.RemoveDynamic(this, &ARandomNPCActor::OnBeginOverlap);
}

void ARandomNPCActor::TickActor(float DeltaTime, 
								enum ELevelTick TickType, 
								FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	if (mIsDead)
		return;

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

void ARandomNPCActor::MoveInDirection(EMoveDirection Direction, 
									  float DeltaTime)
{
	FVector MovementVector;
	switch (Direction)
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
	AddActorWorldOffset(MovementVector * mMoveSpeed * DeltaTime, true, &Hit);
}

void ARandomNPCActor::Die()
{
	UE_LOG(LogTemp, Display, TEXT("NPC has Died!"));

	mIsDead = true;

	if (mOnResetCallback)
		mOnResetCallback();
}

void ARandomNPCActor::FoundTreasure()
{
	UE_LOG(LogTemp, Display, TEXT("NPC Found Treasure!"));

	if (mOnResetCallback)
		mOnResetCallback();
}

void ARandomNPCActor::FoundCoin(AActor* actor)
{
	UE_LOG(LogTemp, Display, TEXT("NPC Found Coin!"));

	actor->Destroy();
}

void ARandomNPCActor::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, 
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
