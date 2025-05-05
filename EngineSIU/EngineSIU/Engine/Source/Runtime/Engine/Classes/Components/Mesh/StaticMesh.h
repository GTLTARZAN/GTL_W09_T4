#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

struct FStaticMeshRenderData;
struct FMaterial;

class UStaticMesh : public UObject
{
    DECLARE_CLASS(UStaticMesh, UObject)

public:
    UStaticMesh() = default;

    virtual UObject* Duplicate(UObject* InOuter) override;

    const TArray<FMaterial*>& GetMaterials() const { return materials; }
    uint32 GetMaterialIndex(FName MaterialSlotName) const;
    void GetUsedMaterials(TArray<UMaterial*>& OutMaterial) const;
    FStaticMeshRenderData* GetRenderData() const { return RenderData; }

    //ObjectName은 경로까지 포함
    FWString GetObjectName() const;

    void SetData(FStaticMeshRenderData* InRenderData);

private:
    FStaticMeshRenderData* RenderData = nullptr;
    TArray<FMaterial*> materials;
};
