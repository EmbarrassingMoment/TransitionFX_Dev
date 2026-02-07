#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TransitionPreset.h"
#include "ITransitionEffect.h"
#include "TransitionManagerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransitionStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransitionCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTransitionHoldStarted);

class APlayerController;

/** Pool for transition effects. */
USTRUCT()
struct FTransitionEffectPool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UObject>> Effects;
};

UENUM(BlueprintType)
enum class ETransitionMode : uint8
{
	Forward UMETA(DisplayName = "Fade Out (0 to 1)"),
	Reverse UMETA(DisplayName = "Fade In (1 to 0)")
};

/**
 * Manager subsystem for transition effects.
 */
UCLASS()
class TRANSITIONFX_API UTransitionManagerSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// FTickableGameObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override;
	virtual TStatId GetStatId() const override;

	// USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Starts a transition with the given preset. */
	UFUNCTION(BlueprintCallable, Category = "Transition")
	void StartTransition(UTransitionPreset* Preset, ETransitionMode Mode = ETransitionMode::Forward, float PlaySpeed = 1.0f, bool bInvert = false, bool bHoldAtMax = false, FTransitionParameters OverrideParams = FTransitionParameters());

	/** Releases the hold at max progress, allowing the transition to complete. */
	UFUNCTION(BlueprintCallable, Category = "Transition")
	void ReleaseHold();

	/** Sets the playback speed multiplier. */
	UFUNCTION(BlueprintCallable, Category = "Transition")
	void SetPlaySpeed(float NewSpeed);

	/** Reverses the current transition. */
	UFUNCTION(BlueprintCallable, Category = "Transition")
	void ReverseTransition(bool bAutoStop = true);

	/** Stops the current transition. */
	UFUNCTION(BlueprintCallable, Category = "Transition")
	void StopTransition();

	/** Returns true if a transition is currently playing. */
	UFUNCTION(BlueprintPure, Category = "Transition")
	bool IsTransitionPlaying() const;

	/** Returns true if the current transition has finished its hold phase or completed. */
	UFUNCTION(BlueprintPure, Category = "Transition")
	bool IsCurrentTransitionFinished() const;

	/** Returns the current progress of the transition (0.0 to 1.0). */
	UFUNCTION(BlueprintPure, Category = "Transition")
	float GetCurrentProgress() const;

	/** Forcefully clears any active transition and resets input. */
	UFUNCTION(BlueprintCallable, Category = "Transition")
	void ForceClear();

	/** Returns the default Fade preset (DA_FadeToBlack), loading it if necessary. */
	UTransitionPreset* GetDefaultFadePreset();

	/** Returns the default master material (M_Transition_Master), loading it if necessary. */
	UMaterialInterface* GetDefaultMasterMaterial();

	/** Preloads a list of transition presets to warm up shaders. */
	UFUNCTION(BlueprintCallable, Category = "TransitionFX|System")
	void PreloadTransitionPresets(const TArray<UTransitionPreset*>& Presets);

public:
	/** Triggered when a transition starts. */
	UPROPERTY(BlueprintAssignable, Category = "Transition")
	FOnTransitionStarted OnTransitionStarted;

	/** Triggered when a transition completes. */
	UPROPERTY(BlueprintAssignable, Category = "Transition")
	FOnTransitionCompleted OnTransitionCompleted;

	/** Triggered when a transition holds at max progress (1.0). */
	UPROPERTY(BlueprintAssignable, Category = "Transition")
	FTransitionHoldStarted OnTransitionHoldStarted;

private:
	/** Pool of available transition effects. */
	UPROPERTY(Transient)
	TMap<UClass*, FTransitionEffectPool> EffectPool;

	/** The current transition preset. */
	UPROPERTY(Transient)
	TObjectPtr<UTransitionPreset> CurrentPreset;

	/** The current active effect instance. */
	UPROPERTY(Transient)
	TScriptInterface<ITransitionEffect> CurrentEffect;

	/** Current transition mode. */
	ETransitionMode CurrentMode;

	/** Current normalized progress of the transition (0.0 to 1.0). */
	float CurrentProgress;

	/** Whether a transition is currently active. */
	bool bIsTransitionActive;

	/** Whether to automatically stop the transition when reverse completes. */
	bool bAutoStopOnReverseComplete;

	/** Whether the completion event has been triggered for the current transition. */
	bool bHasCompleted;

	/** Whether the transition should hold when it reaches max progress (1.0). */
	bool bShouldHoldAtMax;

	/** Whether the transition is currently holding at max progress. */
	bool bIsHolding;

	/** Current playback speed multiplier. */
	float CurrentPlaySpeed;

	/** Cached player controller to avoid redundant lookups. */
	TWeakObjectPtr<APlayerController> CachedPlayerController;

	/** Cached default fade preset to avoid runtime loading. */
	UPROPERTY(Transient)
	TObjectPtr<UTransitionPreset> DefaultFadePreset;

	/** Cached default master material to avoid runtime loading. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DefaultMasterMaterial;
};
