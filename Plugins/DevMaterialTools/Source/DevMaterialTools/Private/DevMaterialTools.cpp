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

namespace
{
	/**
	 * Calls Visitor(Input, bAllowConstant) with the root input backing Property,
	 * for exactly the properties UMaterialGraphNode_Root::UpdateInputUseConstant
	 * (UnrealEd/Private/MaterialGraphNode_Root.cpp) handles; keep the two in sync.
	 * bAllowConstant is false where the editor pins UseConstant to false
	 * (UE-219232). Other properties (ShadingModel, FrontMaterial,
	 * MaterialAttributes, ...) are skipped, as in the editor.
	 */
	template <typename VisitorType>
	void VisitRootInput(UMaterialEditorOnlyData& Data, EMaterialProperty Property, VisitorType&& Visitor)
	{
		switch (Property)
		{
		case MP_EmissiveColor:       Visitor(Data.EmissiveColor, true); break;
		case MP_Opacity:             Visitor(Data.Opacity, true); break;
		case MP_OpacityMask:         Visitor(Data.OpacityMask, true); break;
		case MP_BaseColor:           Visitor(Data.BaseColor, true); break;
		case MP_Metallic:            Visitor(Data.Metallic, true); break;
		case MP_Specular:            Visitor(Data.Specular, true); break;
		case MP_Roughness:           Visitor(Data.Roughness, true); break;
		case MP_Normal:              Visitor(Data.Normal, true); break;
		case MP_Tangent:             Visitor(Data.Tangent, true); break;
		case MP_SubsurfaceColor:     Visitor(Data.SubsurfaceColor, true); break;
		case MP_CustomData0:         Visitor(Data.ClearCoat, true); break;
		case MP_CustomData1:         Visitor(Data.ClearCoatRoughness, true); break;
		case MP_AmbientOcclusion:    Visitor(Data.AmbientOcclusion, true); break;
		case MP_Refraction:          Visitor(Data.Refraction, true); break;
		case MP_SurfaceThickness:    Visitor(Data.SurfaceThickness, true); break;
		case MP_PixelDepthOffset:    Visitor(Data.PixelDepthOffset, false); break;
		case MP_Anisotropy:          Visitor(Data.Anisotropy, false); break;
		case MP_WorldPositionOffset: Visitor(Data.WorldPositionOffset, false); break;
		case MP_Displacement:        Visitor(Data.Displacement, false); break;
		default:
			if (Property >= MP_CustomizedUVs0 && Property <= MP_CustomizedUVs7)
			{
				Visitor(Data.CustomizedUVs[Property - MP_CustomizedUVs0], true);
			}
			break;
		}
	}
}

int32 UDevMaterialTools::SyncRootInputUseConstant(UMaterial* Material)
{
	UMaterialEditorOnlyData* Data = Material ? Material->GetEditorOnlyData() : nullptr;
	if (!Data)
	{
		return 0;
	}

	Material->PreEditChange(nullptr);
	int32 Changed = 0;
	for (int32 Index = 0; Index < MP_MAX; ++Index)
	{
		VisitRootInput(*Data, static_cast<EMaterialProperty>(Index), [&Changed](auto& Input, bool bAllowConstant)
		{
			const bool bUseConstant = bAllowConstant && Input.Expression == nullptr;
			if ((Input.UseConstant != 0) != bUseConstant)
			{
				Input.UseConstant = bUseConstant;
				++Changed;
			}
		});
	}
	Material->PostEditChange();
	Material->MarkPackageDirty();
	return Changed;
}

bool UDevMaterialTools::GetRootInputUseConstant(UMaterial* Material, EMaterialProperty Property)
{
	UMaterialEditorOnlyData* Data = Material ? Material->GetEditorOnlyData() : nullptr;
	bool bUseConstant = false;
	if (Data)
	{
		VisitRootInput(*Data, Property, [&bUseConstant](auto& Input, bool)
		{
			bUseConstant = Input.UseConstant != 0;
		});
	}
	return bUseConstant;
}
