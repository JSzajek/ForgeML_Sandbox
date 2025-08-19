#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EMoveDirection : uint8 
{
	None,
	Forward,
	Backward,
	Left,
	Right,

	COUNT
};