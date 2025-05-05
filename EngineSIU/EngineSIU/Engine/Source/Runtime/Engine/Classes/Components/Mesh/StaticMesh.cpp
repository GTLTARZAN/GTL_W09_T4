#include "StaticMesh.h"
#include "Engine/FObjLoader.h"
#include "UObject/Casts.h"
#include "UObject/ObjectFactory.h"

#include "Engine/Asset/StaticMeshAsset.h"

UObject* UStaticMesh::Duplicate(UObject* InOuter)
{
    // TODO: Context->CopyResource를 사용해서 Buffer복사
    // ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate());
    return nullptr;
}

uint32 UStaticMesh::GetMaterialIndex(FName MaterialSlotName) const
{
    for (uint32 materialIndex = 0; materialIndex < materials.Num(); materialIndex++) {
        if (materials[materialIndex]->MaterialSlotName == MaterialSlotName)
            return materialIndex;
    }

    return -1;
}

void UStaticMesh::GetUsedMaterials(TArray<UMaterial*>& OutMaterial) const
{
    for (const FMaterial* Material : materials)
    {
        OutMaterial.Emplace(Material->Material);
    }
}

FWString UStaticMesh::GetObjectName() const
{
    return RenderData->ObjectName;
}

void UStaticMesh::SetData(FStaticMeshRenderData* InRenderData)
{
    RenderData = InRenderData;

    for (int materialIndex = 0; materialIndex < RenderData->Materials.Num(); materialIndex++)
    {
        FMaterial* newMaterialSlot = new FMaterial();
        UMaterial* newMaterial = FResourceManager::CreateMaterial(RenderData->Materials[materialIndex]);

        newMaterialSlot->Material = newMaterial;
        newMaterialSlot->MaterialSlotName = RenderData->Materials[materialIndex].MaterialName;

        materials.Add(newMaterialSlot);
    }
}
