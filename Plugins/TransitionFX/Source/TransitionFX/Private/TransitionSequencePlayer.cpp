// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionSequencePlayer.h"
#include "TransitionManagerSubsystem.h"
#include "TransitionSequence.h"
#include "TransitionFXConfig.h"
#include "TransitionFX.h"
#include "Engine/World.h"
#include "TimerManager.h"

/** Returns the owning subsystem (this player's Outer). */
UTransitionManagerSubsystem* UTransitionSequencePlayer::GetManager() const
{
	return CastChecked<UTransitionManagerSubsystem>(GetOuter());
}

/** Starts playback from the first entry of an already-validated sequence. */
void UTransitionSequencePlayer::Play(UTransitionSequence* Sequence)
{
	CurrentSequence = Sequence;
	CurrentSequenceStep = INDEX_NONE;
	CurrentLoopIteration = 0;
	bIsSequencePlaying = true;

	StartSequenceStep(0);
}

/**
 * Begins the entry at StepIndex, handles loop wrap-around / completion, and skips
 * null-preset entries with a warning.
 */
void UTransitionSequencePlayer::StartSequenceStep(int32 StepIndex)
{
	if (!CurrentSequence || !bIsSequencePlaying)
	{
		return;
	}

	const int32 NumEntries = CurrentSequence->Entries.Num();

	// Loop / completion handling when we run off the end.
	if (StepIndex >= NumEntries)
	{
		if (!CurrentSequence->bLoop)
		{
			FinishSequence();
			return;
		}

		CurrentLoopIteration++;

		const int32 LoopCount = CurrentSequence->LoopCount;
		if (LoopCount > 0 && CurrentLoopIteration > LoopCount)
		{
			FinishSequence();
			return;
		}

		StepIndex = 0;
	}

	const FTransitionSequenceEntry& Entry = CurrentSequence->Entries[StepIndex];

	if (!Entry.Preset)
	{
		UE_LOG(LogTransitionFX, Warning, TEXT("PlaySequence: Entry %d has a null preset. Skipping."), StepIndex);
		StartSequenceStep(StepIndex + 1);
		return;
	}

	UTransitionManagerSubsystem* Manager = GetManager();

	CurrentSequenceStep = StepIndex;
	Manager->OnSequenceStepChanged.Broadcast(StepIndex);

	const float SafeOverride = FMath::Max(0.0f, Entry.DurationOverride);
	const float TargetDuration = (SafeOverride > 0.0f) ? SafeOverride : Entry.Preset->DefaultDuration;
	const float PlaySpeed = TransitionFXConfig::CalculatePlaySpeed(Entry.Preset->DefaultDuration, TargetDuration);

	// One-shot bind: remove any stale binding before adding to prevent duplicates.
	Manager->OnTransitionCompleted.RemoveDynamic(this, &UTransitionSequencePlayer::OnSequenceStepFinished);
	Manager->OnTransitionCompleted.AddDynamic(this, &UTransitionSequencePlayer::OnSequenceStepFinished);

	// Scope-limit the internal-dispatch flag so StartTransition's sequence guard
	// lets our own per-step call through without aborting the sequence.
	TGuardValue<bool> DispatchGuard(bIsDispatchingSequenceStep, true);
	Manager->StartTransition(Entry.Preset, Entry.Mode, PlaySpeed, Entry.bInvert, /*bHoldAtMax=*/false, Entry.OverrideParams);
}

/**
 * Fired when the current entry's transition completes. Advances to the next entry
 * after DelayAfter (if any).
 */
void UTransitionSequencePlayer::OnSequenceStepFinished()
{
	// One-shot: always remove immediately to keep sequence step transitions deterministic.
	GetManager()->OnTransitionCompleted.RemoveDynamic(this, &UTransitionSequencePlayer::OnSequenceStepFinished);

	if (!bIsSequencePlaying || !CurrentSequence)
	{
		return;
	}

	if (!CurrentSequence->Entries.IsValidIndex(CurrentSequenceStep))
	{
		FinishSequence();
		return;
	}

	const float DelayAfter = FMath::Max(0.0f, CurrentSequence->Entries[CurrentSequenceStep].DelayAfter);
	const int32 NextIndex = CurrentSequenceStep + 1;

	UWorld* World = GetManager()->GetWorld();
	FTimerDelegate Delegate = FTimerDelegate::CreateWeakLambda(this, [this, NextIndex]()
	{
		StartSequenceStep(NextIndex);
	});

	// Defer advancement via the timer manager so each step starts on a fresh tick
	// (avoids unbounded recursion for null-preset skips and keeps frame timing predictable).
	if (!World)
	{
		StartSequenceStep(NextIndex);
		return;
	}

	if (DelayAfter <= 0.0f)
	{
		World->GetTimerManager().SetTimerForNextTick(Delegate);
	}
	else
	{
		World->GetTimerManager().SetTimer(SequenceDelayTimerHandle, Delegate, DelayAfter, false);
	}
}

/**
 * Resets playback state, tears down the final step's effect, and broadcasts
 * OnSequenceCompleted. The subsystem's Tick skips its usual auto-stop while a
 * sequence is playing to prevent background frames between steps, so the final
 * effect is still active here and must be cleaned up explicitly.
 */
void UTransitionSequencePlayer::FinishSequence()
{
	UTransitionManagerSubsystem* Manager = GetManager();

	Manager->OnTransitionCompleted.RemoveDynamic(this, &UTransitionSequencePlayer::OnSequenceStepFinished);

	if (UWorld* World = Manager->GetWorld())
	{
		World->GetTimerManager().ClearTimer(SequenceDelayTimerHandle);
	}

	CurrentSequence = nullptr;
	CurrentSequenceStep = INDEX_NONE;
	CurrentLoopIteration = 0;
	bIsSequencePlaying = false;

	// Clean up the final step's effect now that the sequence is complete.
	if (Manager->IsTransitionPlaying())
	{
		Manager->StopTransition();
	}

	Manager->OnSequenceCompleted.Broadcast();
}

/**
 * Stops the active sequence mid-flight. Does NOT broadcast OnSequenceCompleted
 * (cancellation is not a successful completion).
 */
void UTransitionSequencePlayer::Stop()
{
	if (!bIsSequencePlaying)
	{
		return;
	}

	UTransitionManagerSubsystem* Manager = GetManager();

	if (UWorld* World = Manager->GetWorld())
	{
		World->GetTimerManager().ClearTimer(SequenceDelayTimerHandle);
	}

	Manager->OnTransitionCompleted.RemoveDynamic(this, &UTransitionSequencePlayer::OnSequenceStepFinished);

	// Reset sequence state BEFORE stopping the underlying transition so that any
	// re-entrant callbacks see a clean state.
	CurrentSequence = nullptr;
	CurrentSequenceStep = INDEX_NONE;
	CurrentLoopIteration = 0;
	bIsSequencePlaying = false;

	if (Manager->IsTransitionPlaying())
	{
		Manager->StopTransition();
	}
}

/**
 * Clears all playback state without touching the underlying transition.
 * The subsystem's ForceClear / Deinitialize handles effect cleanup itself.
 */
void UTransitionSequencePlayer::Reset()
{
	UTransitionManagerSubsystem* Manager = GetManager();

	if (UWorld* World = Manager->GetWorld())
	{
		World->GetTimerManager().ClearTimer(SequenceDelayTimerHandle);
	}

	Manager->OnTransitionCompleted.RemoveDynamic(this, &UTransitionSequencePlayer::OnSequenceStepFinished);

	CurrentSequence = nullptr;
	CurrentSequenceStep = INDEX_NONE;
	CurrentLoopIteration = 0;
	bIsSequencePlaying = false;
}
