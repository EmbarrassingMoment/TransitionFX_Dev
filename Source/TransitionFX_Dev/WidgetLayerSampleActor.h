#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WidgetLayerSampleActor.generated.h"

class UUserWidget;

/**
 * Placed in L_WidgetLayerSample: on BeginPlay it creates WidgetClass, adds it to the
 * viewport and switches the player controller to mouse-driven UI input.
 * Stands in for a Level Blueprint so the level can be generated headlessly.
 */
UCLASS()
class TRANSITIONFX_DEV_API AWidgetLayerSampleActor : public AActor
{
	GENERATED_BODY()

public:
	AWidgetLayerSampleActor();

	/** Widget shown on BeginPlay (normally WBP_WidgetLayerSample). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX|Sample")
	TSubclassOf<UUserWidget> WidgetClass;

	/** ZOrder passed to AddToViewport. Keep it below UTransitionPreset::WidgetZOrder (default 10000). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TransitionFX|Sample")
	int32 WidgetZOrder = 0;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> SpawnedWidget;
};
