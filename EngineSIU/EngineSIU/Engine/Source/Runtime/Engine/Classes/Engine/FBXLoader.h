#pragma once
#include <memory>
#include <fbxsdk.h>

#include "Asset/SkeletonAsset.h"
#include "Container/Array.h"
#include "Container/Map.h"

class USkeletalMesh;
class FResourceManager;

struct FSkeletalMeshRenderData;
struct FResourceManager;

struct FFBXLoader
{
    static FMatrix FbxAMatrixToFMatrix(const FbxAMatrix& Src);
    static void ParseSkeleton(FSkeleton& OutSkeleton);
    static void ParseMesh(FbxNode* Node, FSkeletalMeshRenderData& SkeletonData);
    // static void ParseMesh(FbxNode* Node, FSkinMesh& OutSkinMesh, const FSkeleton& Skeleton);
    static void TraverseNodes(FbxNode* Node, FSkeletalMeshRenderData& SkeletonData);
    static void ParseFBX(FSkeletalMeshRenderData& SkeletonData);
    static void LoadSkeletalMesh(const FString& FilePath, FSkeletalMeshRenderData& OutSkeleton);
    static void ParseMaterials(FbxNode* Node, TArray<FObjMaterialInfo>& OutMaterials);
    static bool CreateTextureFromFile(const FWString& Filename, bool bIsSRGB);

private:
    inline static TMap<std::string, uint32> VertexMap; // 중복 체크용
    
    inline static std::shared_ptr<FbxManager> FbxLoadManager{
        FbxManager::Create(),
        [](FbxManager* InManager) {InManager->Destroy();} //커스텀 삭제자
    };
    
    inline static std::shared_ptr<FbxScene> FbxLoadScene{
        FbxScene::Create(FbxLoadManager.get(), "ResourceManager"),
        [](FbxScene* InScene) {InScene->Destroy();}
    };
};
