// Fill out your copyright notice in the Description page of Project Settings.


#include "Camera/CombatCameraInstanceSubSystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

void UCombatCameraInstanceSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (bLogDebug)
	{
		UE_LOG(LogTemp,Log, TEXT("[CameraDirector] Initialize"))
	}
}

void UCombatCameraInstanceSubSystem::Deinitialize()
{
	// Clean up all registered data
	Shots.Empty();
	FocusTargets.Empty();
	Super::Deinitialize();
}

APlayerController* UCombatCameraInstanceSubSystem::GetPC0() const
{
	// Return PlayerController 0 for single player
   // In multiplayer need a specific PlayerControllers instead Not Sure if we are using PC0
	if (UWorld* World = GetWorld())
	{
		return UGameplayStatics::GetPlayerController(World, 0);
	}
	return nullptr;
}



void UCombatCameraInstanceSubSystem::RegisterShot(FName ShotId, AActor* ShotActor)
{
	if (!ShotActor)return;

	// Store as weak pointer (safe if Actor gets destroyed)
	Shots.Add(ShotId, ShotActor);
	if (bLogDebug)
	{
		UE_LOG(LogTemp, Log, TEXT("[CameraDirector] Registered Shot: %s"), *ShotId.ToString());
	}

}

void UCombatCameraInstanceSubSystem::RequestShot(FName ShotId, float BlendTime)
{
	// Try to find the shot in the map
	TWeakObjectPtr<AActor>* Found = Shots.Find(ShotId);
	if (!Found || !Found->IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CameraDirector] ShotId NOT FOUND: %s"), *ShotId.ToString());
		return;
	}

	if (APlayerController* PC = GetPC0())
	{
		// Configure blend parameters for a smooth transition
		// (Basic settings now, but editable in the future)
		FViewTargetTransitionParams Params;
		Params.BlendTime = BlendTime;
		Params.BlendFunction = EViewTargetBlendFunction::VTBlend_Cubic;

		// Track current/last shot IDs
		LastShotId = CurrentShotId;
		CurrentShotId = ShotId;

		// Perform the camera transition
		PC->SetViewTarget(Found->Get(), Params);

		if (bLogDebug)
		{
			UE_LOG(LogTemp, Log, TEXT("[CameraDirector] RequestShot -> %s (Blend=%.2f)"),
				*ShotId.ToString(), BlendTime);
		}
	}
}

void UCombatCameraInstanceSubSystem::RequestOverview()
{
	// (Always request the overview shot)
	RequestShot(OverviewShotId, 1.0f);
}

void UCombatCameraInstanceSubSystem::BuildFocusList()
{
	FocusTargets.Reset();
	if (UWorld* World = GetWorld())
	{
		// Find all actors in the level with the configured CombatantTag
		// (We can use multiple tags)
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsWithTag(World, CombatantTag, FoundActors);
		for (AActor* A : FoundActors)
		{
			FocusTargets.Add(A);
		}
		CurrentFocusIndex = -1;

		if (bLogDebug)
		{
			UE_LOG(LogTemp, Log, TEXT("[CameraDirector] FocusTargets = %d"), FocusTargets.Num());
		}
	}
}

void UCombatCameraInstanceSubSystem::CycleNextTarget()
{
	// If no targets collected yet, rebuild the list
	if (FocusTargets.Num() == 0)
	{
		BuildFocusList();
	}
	// Still empty → nothing to focus
	if (FocusTargets.Num() == 0) return;

	// Move to next index (wrap around using modulo)
	CurrentFocusIndex = (CurrentFocusIndex + 1) % FocusTargets.Num();

	// Get the target(TargetActor) from the weak pointer
	AActor* Target = FocusTargets[CurrentFocusIndex].Get();
	RequestFocus(Target, 0.6f);
}

void UCombatCameraInstanceSubSystem::RequestFocus(AActor* Target, float BlendTime)
{
	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CameraDirector] RequestFocus: Target is null"));
		return;
	}

	// For now, only log which actor is being focused.
	// Later i will plug this into a BP_CamShot_TargetOrbit,
	// set its TargetRef = Target, and then call RequestShot("Orbit_A").
	// TODO: Add the orbit 
	if (bLogDebug)
	{
		UE_LOG(LogTemp, Log, TEXT("[CameraDirector] RequestFocus -> %s"), *Target->GetName());
	}

	// TEMP: fallback overview until orbit shots exist
	RequestOverview();
}

