#include "WidgetLayerSampleActor.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

AWidgetLayerSampleActor::AWidgetLayerSampleActor()
{
	PrimaryActorTick.bCanEverTick = false;
	// A root component gives the actor a transform so it can be placed and selected in the level.
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AWidgetLayerSampleActor::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no player controller, sample widget not shown."), *GetName());
		return;
	}
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: WidgetClass is not set."), *GetName());
		return;
	}

	SpawnedWidget = CreateWidget<UUserWidget>(PlayerController, WidgetClass);
	if (!SpawnedWidget)
	{
		return;
	}
	SpawnedWidget->AddToViewport(WidgetZOrder);

	PlayerController->bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(SpawnedWidget->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);
}
