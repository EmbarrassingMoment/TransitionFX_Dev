// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/TimerHandle.h"
#include "TransitionSequencePlayer.generated.h"

class UTransitionManagerSubsystem;
class UTransitionSequence;

/**
 * Internal playback engine for UTransitionSequence assets.
 *
 * Owned by UTransitionManagerSubsystem (its Outer), which remains the public
 * Blueprint-facing API: the subsystem validates and forwards PlaySequence /
 * StopSequence calls here, while this player drives the subsystem's
 * StartTransition per entry and broadcasts the subsystem's sequence delegates
 * (OnSequenceStepChanged / OnSequenceCompleted).
 */
UCLASS()
class TRANSITIONFX_API UTransitionSequencePlayer : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Starts playback from the first entry. The owning subsystem has already
	 * validated the sequence (non-null, non-empty) and stopped any prior
	 * transition or sequence.
	 */
	void Play(UTransitionSequence* Sequence);

	/**
	 * Stops playback mid-flight and stops the underlying transition.
	 * Does NOT broadcast OnSequenceCompleted (cancellation is not a successful completion).
	 */
	void Stop();

	/**
	 * Clears all playback state (delay timer, delegate binding, current sequence)
	 * WITHOUT touching the underlying transition or broadcasting delegates.
	 * Used by the subsystem's ForceClear / Deinitialize teardown, which handles
	 * the effect cleanup itself.
	 */
	void Reset();

	/** Returns true while a sequence is in progress. */
	bool IsPlaying() const { return bIsSequencePlaying; }

	/** Returns the index of the currently playing entry, or -1 when not playing. */
	int32 GetCurrentStep() const { return bIsSequencePlaying ? CurrentSequenceStep : INDEX_NONE; }

	/**
	 * True only while this player is dispatching a per-entry StartTransition call.
	 * Lets the subsystem's StartTransition distinguish internal sequence-driven
	 * calls from external callers that should interrupt the sequence.
	 */
	bool IsDispatchingStep() const { return bIsDispatchingSequenceStep; }

private:
	/** Returns the owning subsystem (this player's Outer). */
	UTransitionManagerSubsystem* GetManager() const;

	/** Begins the entry at StepIndex, or finishes/loops the sequence if out of range. */
	void StartSequenceStep(int32 StepIndex);

	/** Bound one-shot to the manager's OnTransitionCompleted for each entry. Advances to the next step (with optional DelayAfter). */
	UFUNCTION()
	void OnSequenceStepFinished();

	/** Resets playback state, stops the final step's transition, and broadcasts OnSequenceCompleted. */
	void FinishSequence();

	/** The currently playing sequence, or null when no sequence is active. */
	UPROPERTY(Transient)
	TObjectPtr<UTransitionSequence> CurrentSequence = nullptr;

	/** Index of the entry currently playing, or -1 when no sequence is active. */
	int32 CurrentSequenceStep = INDEX_NONE;

	/** Number of completed loop iterations (0 == on the first pass). */
	int32 CurrentLoopIteration = 0;

	/** True while a sequence is in progress. */
	bool bIsSequencePlaying = false;

	/**
	 * Scope-limited flag set by StartSequenceStep while dispatching the per-entry
	 * StartTransition call. Queried by the subsystem via IsDispatchingStep().
	 */
	bool bIsDispatchingSequenceStep = false;

	/** Timer handle for DelayAfter between entries. */
	FTimerHandle SequenceDelayTimerHandle;
};
