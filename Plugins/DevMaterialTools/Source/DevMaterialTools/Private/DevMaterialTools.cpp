#include "DevMaterialTools.h"

#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionComment.h"
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

namespace
{
	UMaterialExpressionComment* NewComment(UObject* Outer, const FString& Text, int32 NodePosX, int32 NodePosY, int32 SizeX, int32 SizeY, const FLinearColor& Color)
	{
		UMaterialExpressionComment* Comment = NewObject<UMaterialExpressionComment>(Outer, NAME_None, RF_Transactional);
		Comment->MaterialExpressionEditorX = NodePosX;
		Comment->MaterialExpressionEditorY = NodePosY;
		Comment->SizeX = SizeX;
		Comment->SizeY = SizeY;
		Comment->Text = Text;
		Comment->CommentColor = Color;
		return Comment;
	}
}

UMaterialExpressionComment* UDevMaterialTools::CreateCommentInMaterial(UMaterial* Material, const FString& Text, int32 NodePosX, int32 NodePosY, int32 SizeX, int32 SizeY, FLinearColor Color)
{
	if (!Material)
	{
		return nullptr;
	}

	UMaterialExpressionComment* Comment = NewComment(Material, Text, NodePosX, NodePosY, SizeX, SizeY, Color);
	Comment->Material = Material;
	Material->GetExpressionCollection().AddComment(Comment);
	Material->MarkPackageDirty();
	return Comment;
}

UMaterialExpressionComment* UDevMaterialTools::CreateCommentInFunction(UMaterialFunction* MaterialFunction, const FString& Text, int32 NodePosX, int32 NodePosY, int32 SizeX, int32 SizeY, FLinearColor Color)
{
	if (!MaterialFunction)
	{
		return nullptr;
	}

	UMaterialExpressionComment* Comment = NewComment(MaterialFunction, Text, NodePosX, NodePosY, SizeX, SizeY, Color);
	MaterialFunction->GetExpressionCollection().AddComment(Comment);
	MaterialFunction->MarkPackageDirty();
	return Comment;
}

namespace
{
	int32 ClearComments(FMaterialExpressionCollection& Collection, UObject* Package)
	{
		TArray<TObjectPtr<UMaterialExpressionComment>> Comments = Collection.EditorComments;
		for (UMaterialExpressionComment* Comment : Comments)
		{
			Collection.RemoveComment(Comment);
			if (Comment)
			{
				Comment->MarkAsGarbage();
			}
		}
		if (Comments.Num() > 0)
		{
			Package->MarkPackageDirty();
		}
		return Comments.Num();
	}
}

int32 UDevMaterialTools::ClearCommentsInMaterial(UMaterial* Material)
{
	return Material ? ClearComments(Material->GetExpressionCollection(), Material) : 0;
}

int32 UDevMaterialTools::ClearCommentsInFunction(UMaterialFunction* MaterialFunction)
{
	return MaterialFunction ? ClearComments(MaterialFunction->GetExpressionCollection(), MaterialFunction) : 0;
}
