#include "BaseDungeonActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"

ABaseDungeonActor::ABaseDungeonActor()
{
	PrimaryActorTick.bCanEverTick = true;

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

void ABaseDungeonActor::BeginPlay()
{
	Super::BeginPlay();

	if (mpCollisionComponent)
		mpCollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ABaseDungeonActor::OnBeginOverlap);
}

void ABaseDungeonActor::BeginDestroy()
{
	Super::BeginDestroy();

	if (mpCollisionComponent)
		mpCollisionComponent->OnComponentBeginOverlap.RemoveDynamic(this, &ABaseDungeonActor::OnBeginOverlap);
}


void ABaseDungeonActor::ResetActor(const FVector& location)
{
	mTime_s = 0;

	SetActorLocation(location);
	SetActorRotation(FRotator::ZeroRotator);

	mVisitedCoins.clear();
}

void ABaseDungeonActor::MoveInDirection(EMoveDirection Direction, 
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

void ABaseDungeonActor::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, 
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
			OnDeath();

		if (OtherActor->ActorHasTag("Treasure"))
			OnFoundTreasure();

		if (OtherActor->ActorHasTag("Coin") && !mVisitedCoins.contains(OtherActor))
		{
			mVisitedCoins.insert(OtherActor);
			OnFoundCoin();
		}
	}

	// Component-level tag check
	if (OtherComp)
	{
		if (OtherComp->ComponentHasTag("Hazard"))
			OnDeath();

		if (OtherComp->ComponentHasTag("Treasure"))
			OnFoundTreasure();

		if (OtherComp->ComponentHasTag("Coin") && !mVisitedCoins.contains(OtherComp->GetOwner()))
		{
			mVisitedCoins.insert(OtherComp->GetOwner());
			OnFoundCoin();
		}
	}
}
