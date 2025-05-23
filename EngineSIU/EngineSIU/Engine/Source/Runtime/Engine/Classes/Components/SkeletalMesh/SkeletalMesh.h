#pragma once

#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "Engine/Source/Runtime/Core/Container/Array.h"
#include "Components/Material/Material.h"
//#include "Engine/Source/Runtime/Engine/Classes/Engine/Asset/SkeletalMeshAsset.h"
#include "Engine/Source/Runtime/Launch/SkeletalDefine.h"

struct FSkeletalMeshRenderData;

class USkeletalMesh : public UObject
{
    DECLARE_CLASS(USkeletalMesh, UObject)

public:
    USkeletalMesh() = default;

    virtual UObject* Duplicate(UObject* InOuter) override;

    USkeletalMesh* DuplicateSkeletalMesh();

    //void InitializeSkeleton(TArray<FBone>& BoneData);

    /** Set the local transform for a specific bone by index */
    void SetBoneLocalTransform(int boneIndex, const FBonePose& localTransform);

    /** Recompute global transforms for all bones */
    void UpdateGlobalTransforms();

    FSkeletalMeshRenderData* GetRenderData() const { return RenderData; }
    const TArray<FStaticMaterial*>& GetMaterials() const { return materials; }
    uint32 GetMaterialIndex(FName MaterialSlotName) const;
    void GetUsedMaterials(TArray<UMaterial*>& OutMaterial) const;
    FSkeleton* GetSkeleton() const;

    void SetData(FSkeletalMeshRenderData* InRenderData);

public:
    //ObjectName은 경로까지 포함
    FWString GetOjbectName() const;


private:
    FSkeletalMeshRenderData* RenderData = nullptr;
    TArray<FStaticMaterial*> materials;
};
