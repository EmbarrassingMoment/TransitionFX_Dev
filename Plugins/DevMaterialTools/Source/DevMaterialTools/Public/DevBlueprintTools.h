#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DevBlueprintTools.generated.h"

class UBlueprint;

/**
 * Local-only helpers for Python-driven Blueprint graph inspection/editing.
 *
 * UBlueprint::UbergraphPages, UEdGraph::Nodes and UEdGraphPin are not
 * reachable from Python (plain UPROPERTY() or non-UObject), so graph pin
 * defaults - e.g. literal arrays on a K2Node_CallFunction input pin - can
 * only be read or rewritten on the C++ side.
 */
UCLASS()
class DEVMATERIALTOOLS_API UDevBlueprintTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns the Level Blueprint of the world's persistent level (loading/creating nothing). */
	UFUNCTION(BlueprintCallable, Category = "DevBlueprintTools")
	static UBlueprint* GetLevelScriptBlueprint(UWorld* World);

	/** Dumps every graph of the Blueprint: nodes, pins, pin defaults and links. */
	UFUNCTION(BlueprintCallable, Category = "DevBlueprintTools")
	static FString DumpBlueprintGraphs(UBlueprint* Blueprint);

	/** Returns the default value string of a pin, located by node object name and pin name. */
	UFUNCTION(BlueprintCallable, Category = "DevBlueprintTools")
	static FString GetPinDefaultValue(UBlueprint* Blueprint, const FString& NodeName, const FString& PinName);

	/** Sets a pin default via the graph schema, marks the Blueprint modified and compiles it. */
	UFUNCTION(BlueprintCallable, Category = "DevBlueprintTools")
	static bool SetPinDefaultValue(UBlueprint* Blueprint, const FString& NodeName, const FString& PinName, const FString& NewValue);

	/**
	 * Writes a soft-object array property directly through the property system.
	 * Needed because set_editor_property refuses BP variables that are not
	 * Instance Editable (CPF_DisableEditOnInstance) on actor instances.
	 */
	UFUNCTION(BlueprintCallable, Category = "DevBlueprintTools")
	static bool SetSoftObjectArrayProperty(UObject* Object, FName PropertyName, const TArray<TSoftObjectPtr<UObject>>& Values);
};
