#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DevMaterialTools.generated.h"

class UMaterial;
class UMaterialFunction;
class UMaterialExpression;
class UMaterialExpressionComment;
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

	/**
	 * Comment boxes need the same treatment as reroute usages: SizeX/SizeY are
	 * plain UPROPERTY() and comments live in the separate EditorComments array,
	 * which CreateMaterialExpression / DeleteAllMaterialExpressions never touch.
	 */
	UFUNCTION(BlueprintCallable, Category = "DevMaterialTools")
	static UMaterialExpressionComment* CreateCommentInMaterial(UMaterial* Material, const FString& Text, int32 NodePosX, int32 NodePosY, int32 SizeX, int32 SizeY, FLinearColor Color);

	UFUNCTION(BlueprintCallable, Category = "DevMaterialTools")
	static UMaterialExpressionComment* CreateCommentInFunction(UMaterialFunction* MaterialFunction, const FString& Text, int32 NodePosX, int32 NodePosY, int32 SizeX, int32 SizeY, FLinearColor Color);

	/** Removes every comment box. Returns the number removed. Pair with delete_all_material_expressions when rebuilding a graph in place. */
	UFUNCTION(BlueprintCallable, Category = "DevMaterialTools")
	static int32 ClearCommentsInMaterial(UMaterial* Material);

	UFUNCTION(BlueprintCallable, Category = "DevMaterialTools")
	static int32 ClearCommentsInFunction(UMaterialFunction* MaterialFunction);
};
