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
   // In multiplayer We need a specific PlayerControllers instead not sure if we are using PC0
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

void UCombatCameraInstanceSubSystem::RequestOverview()
{
	// Uses Overview ID fot Base
	RequestShotID(Overview);
}


void UCombatCameraInstanceSubSystem::BuildTagList()
{
	//Clear Focus target list before building
	FocusTargets.Reset();

	if (UWorld* World = GetWorld())
	{
		TArray<AActor*> FoundActors;

		// Collect actors for every tag in the FocusTags array
		for (const FName& Tag : FocusTags)
		{
			TArray<AActor*> ActorsWithTag;
			UGameplayStatics::GetAllActorsWithTag(World, Tag, ActorsWithTag);
			FoundActors.Append(ActorsWithTag);
		}

		// Remove duplicates (if actors have multiple tags)
		TSet<AActor*> UniqueActors(FoundActors);
		FoundActors = UniqueActors.Array();

		// Store as weak pointers
		for (AActor* A : FoundActors)
		{
			FocusTargets.Add(A);
		}
		CurrentFocusIndex = -1;

		if (bLogDebug)
		{
			UE_LOG(LogTemp, Log, TEXT("[CameraDirector] FocusTargets = %d (tags checked: %d)"),
				FocusTargets.Num(), FocusTags.Num());
		}
	}
}

void UCombatCameraInstanceSubSystem::CycleNextShot(float BlendTime)
{
	if (Shots.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CameraDirector] CycleNextShot: no shots registered"));
		return;
	}

	// Build a sorted array of Ids (deterministic order)
	TArray<FName> SortedIds;
	Shots.GenerateKeyArray(SortedIds);
	SortedIds.Sort(FNameLexicalLess());
	SortedIds.Remove(Overview);
	// Find current index (or -1 if none)
	int32 CurrIdx = SortedIds.IndexOfByKey(CurrentShotId);
	// Next index wraps around
	int32 NextIdx = (CurrIdx + 1) % SortedIds.Num();
	const FName NextId = SortedIds[NextIdx];
	RequestShotID(NextId, BlendTime);
}

void UCombatCameraInstanceSubSystem::CycleNextShotID(FName FocusShotId, float BlendTime)
{
	if (FocusTargets.Num() == 0)
	{
		BuildTagList();
	}

	if (FocusTargets.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CameraDirector] CycleNextTargetWithShot: no focus targets"));
		RequestOverview();
		return;
	}

	const int32 Count = FocusTargets.Num();

	// Ensure the shot exists
	TWeakObjectPtr<AActor>* FoundShot = Shots.Find(FocusShotId);
	if (!FoundShot || !FoundShot->IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CameraDirector] Focus shot '%s' not found. Fallback to Overview."),
			*FocusShotId.ToString());
		RequestOverview();
		return;
	}

	AActor* ShotActor = FoundShot->Get();

	// Try up to Count entries to find a valid Camera
	for (int32 i = 0; i < Count; ++i)
	{
		CurrentFocusIndex = (CurrentFocusIndex + 1) % Count;

		// Skip if invalid
		AActor* Candidate = FocusTargets[CurrentFocusIndex].Get();
		if (!Candidate) continue;                           
		bool bHasValidTag = false;
		for (const FName& Tag : FocusTags)
		{
			if (Candidate->ActorHasTag(Tag))
			{
				bHasValidTag = true;
				break;
			}
		}

		// Skip if has no valid tag
		if (!bHasValidTag) continue;

		// If the shot supports a target param, pass it via BP function SetTargetRef(Target)
		static const FName SetTargetRefFn(TEXT("SetTargetRef"));
		if (UFunction* Fn = ShotActor->FindFunction(SetTargetRefFn))
		{
			struct FSetTargetRefParams { AActor* Target; };
			FSetTargetRefParams Params; Params.Target = Candidate;
			ShotActor->ProcessEvent(Fn, &Params);
		}

		// Blend to the chosen focus shot
		RequestShotID(FocusShotId, BlendTime);

		if (bLogDebug)
		{
			UE_LOG(LogTemp, Log, TEXT("[CameraDirector] CycleNextTargetWithShot -> Target=%s, Shot=%s"),
				*GetNameSafe(Candidate), 
				*FocusShotId.ToString());
		}
		return; // done
	}

	// If we didn’t find a valid candidate, rebuild and fallback
	BuildTagList();
	RequestOverview();

}

bool UCombatCameraInstanceSubSystem::TryBeginBlend(float BlendTime)
{
	//TODO: Make blending logic
	return true;
}

void UCombatCameraInstanceSubSystem::OnBlendFinished()
{
	bIsBlending = false;
	BlendUnlockTime = 0.0;

	if (bLogDebug)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[CameraDirector] Blend finished (unlock)"));
	}
}

void UCombatCameraInstanceSubSystem::ClearTagList()
{
	FocusTargets.Reset();
}

void UCombatCameraInstanceSubSystem::RequestShotActor(AActor* ShotActor, float BlendTime)
{
	if (!TryBeginBlend(BlendTime))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CameraDirector] RequestShot(Actor): Camera is Blending"));
		return;
	}

	if (!ShotActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CameraDirector] RequestShot(Actor): ShotActor is null"));
		return;
	}

	// Try to resolve an Id for tracking (reverse lookup)
	if (APlayerController* PC = GetPC0())
	{
		FName ResolvedId = NAME_None;
		for (const TPair<FName, TWeakObjectPtr<AActor>>& Pair : Shots)
		{
			if (Pair.Value.Get() == ShotActor)
			{
				ResolvedId = Pair.Key;
				break;
			}
		}

		// Blend params
		FViewTargetTransitionParams Params;
		Params.BlendTime = BlendTime;
		Params.BlendFunction = EViewTargetBlendFunction::VTBlend_Cubic;

		// Track Ids if we found one
		LastShotId = CurrentShotId;
		CurrentShotId = ResolvedId;

		// Do the blend
		PC->SetViewTarget(ShotActor, Params);

		if (bLogDebug)
		{
			UE_LOG(LogTemp, Log, TEXT("[CameraDirector] RequestShot(Actor) -> %s (Id=%s, Blend=%.2f)"),
				*GetNameSafe(ShotActor), 
				*ResolvedId.ToString(), 
				BlendTime);
		}
	}
}

void UCombatCameraInstanceSubSystem::RequestShotID(FName ShotId, float BlendTime)
{
	if (!TryBeginBlend(BlendTime))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CameraDirector] RequestShot(Id): Camera is Blending"));
		return;
	}

	// Try to find the shot actor in the map
	TWeakObjectPtr<AActor>* Found = Shots.Find(ShotId);
	if (!Found || !Found->IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CameraDirector] RequestShot(Id): NOT FOUND -> %s"), *ShotId.ToString());
		return;
	}

	RequestShotActor(Found->Get(), BlendTime);
}

