#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

struct FSkeletalMeshData;
struct FMaterial;

class USkeletalMesh : public UObject
{
    DECLARE_CLASS(USkeletalMesh, UObject)

public:
    USkeletalMesh() = default;

    virtual UObject* Duplicate(UObject* InOuter) override;

    const TArray<FMaterial*>& GetMaterials() const { return materials; }
    uint32 GetMaterialIndex(FName MaterialSlotName) const;
    void GetUsedMaterials(TArray<UMaterial*>& OutMaterial) const;
    FSkeletalMeshRenderData* GetRenderData() const { return RenderData; }
    
    //ObjectName은 경로까지 포함
    FWString GetObjectName() const;

    void SetData(FSkeletalMeshRenderData* InRenderData);

private:
    FSkeletalMeshRenderData* RenderData = nullptr;
    TArray<FMaterial*> materials;
};
