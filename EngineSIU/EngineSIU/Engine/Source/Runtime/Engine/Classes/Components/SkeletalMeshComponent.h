#pragma once
#include "Components/MeshComponent.h"
#include "Engine/Asset/SkeletonAsset.h"

class USkeletalMesh;

class USkeletalMeshComponent : public UMeshComponent
{
    DECLARE_CLASS(USkeletalMeshComponent, UMeshComponent)

public:
    USkeletalMeshComponent() = default;

    virtual UObject* Duplicate(UObject* InOuter) override;
    
    void GetProperties(TMap<FString, FString>& OutProperties) const override;
    
    void SetProperties(const TMap<FString, FString>& InProperties) override;

    void SetselectedSubMeshIndex(const int& value) { selectedSubMeshIndex = value; }
    int GetselectedSubMeshIndex() const { return selectedSubMeshIndex; };

    virtual uint32 GetNumMaterials() const override;
    virtual UMaterial* GetMaterial(uint32 ElementIndex) const override;
    virtual uint32 GetMaterialIndex(FName MaterialSlotName) const override;
    virtual TArray<FName> GetMaterialSlotNames() const override;
    virtual void GetUsedMaterials(TArray<UMaterial*>& Out) const override;

    virtual int CheckRayIntersection(const FVector& InRayOrigin, const FVector& InRayDirection, float& OutHitDistance) const override;

    void UpdateBoneGlobalPose();
    void RotateBone(int BoneIndex, FVector Rot);
    void RecursiveUpdateGlobal(int Index);

    USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }
    void SetSkeletalMesh(USkeletalMesh* value);
    
protected:
    int selectedSubMeshIndex = -1;
    USkeletalMesh* SkeletalMesh = nullptr;
    TArray<FMaterial*> materials;
};
