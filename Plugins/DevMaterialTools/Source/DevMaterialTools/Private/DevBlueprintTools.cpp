#include "DevBlueprintTools.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "Engine/Level.h"
#include "Engine/LevelScriptBlueprint.h"
#include "Engine/World.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

namespace
{
	void GetAllGraphs(UBlueprint* Blueprint, TArray<UEdGraph*>& OutGraphs)
	{
		if (Blueprint)
		{
			Blueprint->GetAllGraphs(OutGraphs);
		}
	}

	UEdGraphPin* FindPin(UBlueprint* Blueprint, const FString& NodeName, const FString& PinName, UEdGraphNode*& OutNode)
	{
		OutNode = nullptr;
		TArray<UEdGraph*> Graphs;
		GetAllGraphs(Blueprint, Graphs);
		for (UEdGraph* Graph : Graphs)
		{
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (Node && Node->GetName() == NodeName)
				{
					for (UEdGraphPin* Pin : Node->Pins)
					{
						if (Pin && Pin->GetName() == PinName)
						{
							OutNode = Node;
							return Pin;
						}
					}
				}
			}
		}
		return nullptr;
	}
}

UBlueprint* UDevBlueprintTools::GetLevelScriptBlueprint(UWorld* World)
{
	if (!World || !World->PersistentLevel)
	{
		return nullptr;
	}
	return World->PersistentLevel->GetLevelScriptBlueprint(/*bDontCreate=*/true);
}

FString UDevBlueprintTools::DumpBlueprintGraphs(UBlueprint* Blueprint)
{
	FString Out;
	TArray<UEdGraph*> Graphs;
	GetAllGraphs(Blueprint, Graphs);
	for (UEdGraph* Graph : Graphs)
	{
		Out += FString::Printf(TEXT("GRAPH %s\n"), *Graph->GetName());
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				continue;
			}
			Out += FString::Printf(TEXT("  NODE %s (%s) title='%s'\n"),
				*Node->GetName(), *Node->GetClass()->GetName(),
				*Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (!Pin)
				{
					continue;
				}
				FString Links;
				for (UEdGraphPin* Linked : Pin->LinkedTo)
				{
					if (Linked)
					{
						Links += FString::Printf(TEXT("%s.%s "), *Linked->GetOwningNode()->GetName(), *Linked->GetName());
					}
				}
				Out += FString::Printf(TEXT("    PIN %s dir=%s cat=%s container=%d default='%s' defobj='%s' links=[%s]\n"),
					*Pin->GetName(),
					Pin->Direction == EGPD_Input ? TEXT("in") : TEXT("out"),
					*Pin->PinType.PinCategory.ToString(),
					static_cast<int32>(Pin->PinType.ContainerType),
					*Pin->DefaultValue,
					Pin->DefaultObject ? *Pin->DefaultObject->GetPathName() : TEXT(""),
					*Links);
			}
		}
	}
	return Out;
}

FString UDevBlueprintTools::GetPinDefaultValue(UBlueprint* Blueprint, const FString& NodeName, const FString& PinName)
{
	UEdGraphNode* Node = nullptr;
	if (UEdGraphPin* Pin = FindPin(Blueprint, NodeName, PinName, Node))
	{
		return Pin->DefaultValue;
	}
	return FString();
}

bool UDevBlueprintTools::SetPinDefaultValue(UBlueprint* Blueprint, const FString& NodeName, const FString& PinName, const FString& NewValue)
{
	UEdGraphNode* Node = nullptr;
	UEdGraphPin* Pin = FindPin(Blueprint, NodeName, PinName, Node);
	if (!Pin)
	{
		return false;
	}

	const UEdGraphSchema* Schema = Node->GetGraph()->GetSchema();
	Schema->TrySetDefaultValue(*Pin, NewValue);
	if (Pin->DefaultValue != NewValue)
	{
		return false;
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	return true;
}

bool UDevBlueprintTools::SetSoftObjectArrayProperty(UObject* Object, FName PropertyName, const TArray<TSoftObjectPtr<UObject>>& Values)
{
	if (!Object)
	{
		return false;
	}

	const FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(Object->GetClass(), PropertyName);
	if (!ArrayProp || !CastField<FSoftObjectProperty>(ArrayProp->Inner))
	{
		return false;
	}

	Object->Modify();

	FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Object));
	Helper.Resize(Values.Num());
	for (int32 Index = 0; Index < Values.Num(); ++Index)
	{
		FSoftObjectPtr* ValuePtr = reinterpret_cast<FSoftObjectPtr*>(Helper.GetRawPtr(Index));
		*ValuePtr = FSoftObjectPtr(Values[Index].ToSoftObjectPath());
	}

	Object->MarkPackageDirty();
	return true;
}
