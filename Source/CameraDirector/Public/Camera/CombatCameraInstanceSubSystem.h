// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CombatCameraInstanceSubSystem.generated.h"

/**
 * UCombatCameraInstanceSubSystem
 *
 * A global camera director that manages cinematic shots during combat.
 * Implemented as a GameInstanceSubsystem so it exists once per game session,
 * without needing to place any Actor in the level.
 *
 * Responsibilities:
 * - Register camera shots (Actors with a CameraComponent).
 * - Switch between shots smoothly (SetViewTargetWithBlend).
 * - Build and cycle a list of focusable targets (Camera).
 * - Provide an overview shot (battlefield wide view).
 */

UCLASS(BlueprintType, Blueprintable)
class CAMERADIRECTOR_API UCombatCameraInstanceSubSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**-------------------Subsystem life cicle-------------------**/ 

	 /** Called when the subsystem is created (at game start) **/
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	/** Called when the subsystem is destroyed (at game end). */
	virtual void Deinitialize() override;

	/** ------------------- BlueprintCallable API ------------------- **/

	/** Register a new camera shot by ID. Typically called in BeginPlay of BP_CamShot_* actors **/
	UFUNCTION(BlueprintCallable, Category = "CameraDirector")
	void RegisterShot(FName ShotId, AActor* ShotActor);

	/** Request a camera change to the given Actor, with blend time (default = 0.8s) **/
	UFUNCTION(BlueprintCallable, Category = "CameraDirector")
	void RequestShotActor(AActor* ShotActor, float BlendTime = 0.8f);

	/** Request a camera change to the given ShotId, with blend time (default = 0.8s) **/
	UFUNCTION(BlueprintCallable, Category = "CameraDirector")
	void RequestShotID(FName ShotId, float BlendTime = 0.8f);

	/** Request Default Overview Shot **/
	UFUNCTION(BlueprintCallable, Category = "CameraDirector")
	void RequestOverview(float BlendTime = 0.8f);

	/** Collect all actors with any Tag into the FocusTargets list **/
	UFUNCTION(BlueprintCallable, Category = "CameraDirector")
	void BuildTagList();

	/** Cycle to the next target in FocusTargets and request focus on it **/
	UFUNCTION(BlueprintCallable, Category = "CameraDirector")
	void CycleNextShot(float BlendTime = 0.8f);

	/** Cycle to the next target in FocusTargets with a ID and request focus on it **/

	UFUNCTION(BlueprintCallable, Category = "CameraDirector")
	void CycleNextShotID(FName FocusShotId, float BlendTime = 0.8f);

	/** Return true if we are currently blending */
	UFUNCTION(BlueprintCallable, Category = "CameraDirector|State")
	bool IsBlending() const { return bIsBlending; }

	/** Try to begin a blend; returns false if already blending */
	bool TryBeginBlend(float BlendTime);

	/** Internal: called by timer to clear the blending state */
	void OnBlendFinished();

	/** ------------------- Config Properties ------------------- **/


		/** Clear the FocusTargets list **/
	UFUNCTION(BlueprintCallable, Category = "CameraDirector")
	void ClearTagList();

	 /** Default shot used when calling RequestOverview() **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraDirector")
	FName OverviewShotId = TEXT("Overview_Static_01");

	/** Toggle debug logging in Output Log **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraDirector|Debug")
	bool bLogDebug = true;

	/** Actor Tags used to collect potential focus targets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraDirector|Focus")
	TArray<FName> FocusTags;

private:

	/** Helper function: returns PlayerController 0 (single player case) **/
	/** TODO: Implement the multiplayer version **/
	APlayerController* GetPC0() const;

	/** Map storing all registered shots. Key = ShotId, Value = Shot Actor **/
	UPROPERTY()
	TMap<FName, TWeakObjectPtr<AActor>> Shots;

	/** List of focusable targets (combatants/enemies/etc) **/
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> FocusTargets;

	/** Index of the currently focused target inside FocusTargets **/
	int32 CurrentFocusIndex = -1;

	/** Currently active shot ID **/
	FName CurrentShotId;

	/** Previously active shot ID **/
	FName LastShotId;

	/** Base ID **/
	FName Overview = TEXT("Overview_Static_01");

	/** True while a camera blend is in progress */
	UPROPERTY(VisibleAnywhere, Category = "CameraDirector|State")
	bool bIsBlending = false;

	/** When the current blend is expected to finish (WorldTimeSeconds) */
	double BlendUnlockTime = 0.0;

	/** Timer to unlock bIsBlending when the blend time elapses */
	FTimerHandle BlendTimerHandle;
};
