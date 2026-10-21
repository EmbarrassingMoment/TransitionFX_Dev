// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "TransitionSequence.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "TransitionSequence"

EDataValidationResult UTransitionSequence::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	bool bHasErrors = false;

	if (Entries.Num() == 0)
	{
		Context.AddWarning(LOCTEXT("NoEntries", "The sequence has no entries, so PlaySequence does nothing."));
	}

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const UTransitionPreset* Preset = Entries[Index].Preset;
		if (Preset == nullptr)
		{
			Context.AddError(FText::Format(LOCTEXT("NoPreset", "Entry {0} has no Preset. It is skipped during playback."), FText::AsNumber(Index)));
			bHasErrors = true;
		}
		else if (!Preset->IsPlayable())
		{
			Context.AddError(FText::Format(LOCTEXT("UnplayablePreset", "Entry {0}: Preset '{1}' has no Transition Material or Effect Class. It is skipped during playback."),
				FText::AsNumber(Index), FText::FromString(Preset->GetName())));
			bHasErrors = true;
		}
	}

	if (bHasErrors)
	{
		Result = EDataValidationResult::Invalid;
	}
	else if (Result == EDataValidationResult::NotValidated)
	{
		Result = EDataValidationResult::Valid;
	}
	return Result;
}

#undef LOCTEXT_NAMESPACE
#endif // WITH_EDITOR
