#include "DevMaterialTools.h"

#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialFunction.h"

namespace
{
	UMaterialExpression* LinkUsageToDeclaration(UMaterialExpression* Expression, UMaterialExpressionNamedRerouteDeclaration* Declaration)
	{
		UMaterialExpressionNamedRerouteUsage* Usage = Cast<UMaterialExpressionNamedRerouteUsage>(Expression);
		if (!Usage)
		{
			return nullptr;
		}

		Usage->Declaration = Declaration;
		Usage->DeclarationGuid = Declaration->VariableGuid;
		Usage->MarkPackageDirty();
		return Usage;
	}
}

UMaterialExpression* UDevMaterialTools::CreateNamedRerouteUsage(UMaterial* Material, UMaterialExpressionNamedRerouteDeclaration* Declaration, int32 NodePosX, int32 NodePosY)
{
	if (!Material || !IsValid(Declaration))
	{
		return nullptr;
	}

	UMaterialExpression* Expression = UMaterialEditingLibrary::CreateMaterialExpression(
		Material, UMaterialExpressionNamedRerouteUsage::StaticClass(), NodePosX, NodePosY);
	return LinkUsageToDeclaration(Expression, Declaration);
}

UMaterialExpression* UDevMaterialTools::CreateNamedRerouteUsageInFunction(UMaterialFunction* MaterialFunction, UMaterialExpressionNamedRerouteDeclaration* Declaration, int32 NodePosX, int32 NodePosY)
{
	if (!MaterialFunction || !IsValid(Declaration))
	{
		return nullptr;
	}

	UMaterialExpression* Expression = UMaterialEditingLibrary::CreateMaterialExpressionInFunction(
		MaterialFunction, UMaterialExpressionNamedRerouteUsage::StaticClass(), NodePosX, NodePosY);
	return LinkUsageToDeclaration(Expression, Declaration);
}

FString UDevMaterialTools::GetNamedRerouteUsageDisplayName(UMaterialExpression* UsageExpression)
{
	UMaterialExpressionNamedRerouteUsage* Usage = Cast<UMaterialExpressionNamedRerouteUsage>(UsageExpression);
	// UMaterialExpressionNamedRerouteUsage::IsDeclarationValid() is not ENGINE_API,
	// so it cannot be linked from a project module; IsValid() stands in for it.
	if (!Usage || !IsValid(Usage->Declaration))
	{
		return FString();
	}
	return Usage->Declaration->Name.ToString();
}
