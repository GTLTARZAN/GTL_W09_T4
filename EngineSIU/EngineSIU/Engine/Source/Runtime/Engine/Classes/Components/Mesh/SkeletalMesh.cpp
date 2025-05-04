#include "SkeletalMesh.h"
#include "Engine/FObjLoader.h"

UObject* USkeletalMesh::Duplicate(UObject* InOuter)
{
    return nullptr;
}

uint32 USkeletalMesh::GetMaterialIndex(FName MaterialSlotName) const
{
    for (uint32 materialIndex = 0; materialIndex < Materials.Num(); materialIndex++)
    {
        if (Materials[materialIndex]->MaterialSlotName == MaterialSlotName)
            return materialIndex;
    }

    return -1;
}

void USkeletalMesh::GetUsedMaterials(TArray<UMaterial*>& OutMaterial) const
{
    for (const FStaticMaterial* Material : Materials)
    {
        OutMaterial.Emplace(Material->Material);
    }
}

FWString USkeletalMesh::GetOjbectName() const
{
    return RenderData->ObjectName;
}

void USkeletalMesh::SetData(FSkeletalMeshRenderData* InRenderData)
{
    RenderData = InRenderData;

    for (int materialIndex = 0; materialIndex < RenderData->Materials.Num(); materialIndex++)
    {
        FStaticMaterial* newMaterialSlot = new FStaticMaterial();
        UMaterial* newMaterial = FObjManager::CreateMaterial(RenderData->Materials[materialIndex]);
        newMaterialSlot->Material = newMaterial;
        newMaterialSlot->MaterialSlotName = RenderData->Materials[materialIndex].MaterialName;
        Materials.Add(newMaterialSlot);
    }
}
