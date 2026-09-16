#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WidgetLayerSampleWidget.generated.h"

class UButton;
class UTextBlock;
class UTransitionPreset;
class UTransitionManagerSubsystem;

/**
 * Logic for WBP_WidgetLayerSample (Content/Widget), the control panel of the
 * L_WidgetLayerSample verification level.
 *
 * The panel cycles through the widget-layer presets (DA_Widget_*) and plays either
 * the selected widget-layer preset or its PostProcess counterpart as
 * FadeOut (Forward) -> Hold -> FadeIn (Forward with the mask inverted), i.e. the same
 * flow OpenLevelWithTransition uses. Comparing the two makes the
 * difference visible: a PostProcess transition leaves this UMG panel untouched,
 * a widget-layer transition covers it together with the rest of the screen.
 *
 * The layout lives in the Widget Blueprint; the named widgets below are optional so
 * the designer layout can be rearranged freely.
 */
UCLASS(Abstract, Blueprintable)
class TRANSITIONFX_DEV_API UWidgetLayerSampleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Widget-layer presets (DA_Widget_*) that Prev / Next cycle through. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX|Sample")
	TArray<TSoftObjectPtr<UTransitionPreset>> WidgetPresets;

	/** PostProcess counterparts, index-matched with WidgetPresets. Null entries are allowed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX|Sample")
	TArray<TSoftObjectPtr<UTransitionPreset>> PostProcessPresets;

	/** Seconds to stay fully covered before the inverted fade-in starts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX|Sample", meta = (ClampMin = "0.0"))
	float HoldDuration = 0.3f;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> PrevButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> NextButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> PlayWidgetButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> PlayPostProcessButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PresetNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandlePrevClicked();

	UFUNCTION()
	void HandleNextClicked();

	UFUNCTION()
	void HandlePlayWidgetClicked();

	UFUNCTION()
	void HandlePlayPostProcessClicked();

	UFUNCTION()
	void HandleTransitionHoldStarted();

	UFUNCTION()
	void HandleTransitionCompleted();

	void Play(const TArray<TSoftObjectPtr<UTransitionPreset>>& Presets, const FString& LayerLabel);
	void RefreshPresetLabel();
	void SetStatus(const FString& Status);
	UTransitionManagerSubsystem* GetTransitionManager() const;

	int32 CurrentIndex = 0;
	bool bIsPlaying = false;
	FTimerHandle HoldTimerHandle;
};
