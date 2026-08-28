#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DevMaterialTools.generated.h"

class UMaterial;
class UMaterialFunction;
class UMaterialExpression;
class UMaterialExpressionNamedRerouteDeclaration;

/**
 * Local-only helpers for Python-driven material authoring.
 *
 * Named Reroute Usage nodes can be created through the normal
 * CreateMaterialExpression API, but their Declaration / DeclarationGuid
 * properties are plain UPROPERTY() (neither BlueprintReadWrite nor
 * EditAnywhere), so Python cannot set them and the Usage stays unlinked.
 * These helpers perform the link on the C++ side.
 *
 * BlueprintCallable is what generates the Python bindings; the functions are
 * exposed as unreal.DevMaterialTools.* in snake_case.
 */
UCLASS()
class DEVMATERIALTOOLS_API UDevMaterialTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Creates a Named Reroute Usage node in a Material and links it to Declaration. Returns nullptr on failure. */
	UFUNCTION(BlueprintCallable, Category = "DevMaterialTools")
	static UMaterialExpression* CreateNamedRerouteUsage(UMaterial* Material, UMaterialExpressionNamedRerouteDeclaration* Declaration, int32 NodePosX = 0, int32 NodePosY = 0);

	/** Creates a Named Reroute Usage node in a Material Function and links it to Declaration. Returns nullptr on failure. */
	UFUNCTION(BlueprintCallable, Category = "DevMaterialTools")
	static UMaterialExpression* CreateNamedRerouteUsageInFunction(UMaterialFunction* MaterialFunction, UMaterialExpressionNamedRerouteDeclaration* Declaration, int32 NodePosX = 0, int32 NodePosY = 0);

	/** Returns the name of the declaration a Usage node is linked to, or an empty string if unlinked. For verification from Python. */
	UFUNCTION(BlueprintCallable, Category = "DevMaterialTools")
	static FString GetNamedRerouteUsageDisplayName(UMaterialExpression* UsageExpression);
};
