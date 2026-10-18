#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SceneTypes.h"
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

	/**
	 * Re-syncs every root input's UseConstant flag with its connection state
	 * (UseConstant = Expression == nullptr), mirroring
	 * UMaterialGraphNode_Root::UpdateInputUseConstant, then runs
	 * PreEditChange/PostEditChange so the material recompiles. Returns the
	 * number of inputs whose flag changed.
	 *
	 * The material editor runs that update on every graph relink and the flag is
	 * saved, so a pin that was unconnected when the asset was last saved from the
	 * editor keeps UseConstant = true. ConnectMaterialProperty only sets the
	 * expression, and FScalarMaterialInput::CompileWithDefault prefers the
	 * constant, so the new connection is ignored until the flag is cleared.
	 * UseConstant is a plain bitfield, not a UPROPERTY, so Python cannot reach it.
	 * Call after connect_material_property on duplicated/edited materials.
	 */
	UFUNCTION(BlueprintCallable, Category = "DevMaterialTools")
	static int32 SyncRootInputUseConstant(UMaterial* Material);

	/** Returns a root input's UseConstant flag (false for inputs without one, e.g. ShadingModel). For verification from Python. */
	UFUNCTION(BlueprintCallable, Category = "DevMaterialTools")
	static bool GetRootInputUseConstant(UMaterial* Material, EMaterialProperty Property);
};
