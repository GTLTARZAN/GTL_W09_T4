#pragma once
#include <memory>

#include "Texture.h"
#include "Container/Map.h"

struct FSkeletalMeshRenderData;
struct FObjManager;
struct FObjMaterialInfo;
struct FStaticMeshVertex;
struct FStaticMeshRenderData;

class UMaterial;
class UStaticMesh;
class FRenderer;
class FGraphicsDevice;
class USkeletalMesh;

class FResourceManager
{
#pragma region Texture
public:
    void Initialize(FRenderer* renderer, FGraphicsDevice* device);
    void Release(FRenderer* renderer);
    HRESULT LoadTextureFromFile(ID3D11Device* device, const wchar_t* filename, bool bIsSRGB = true);
    HRESULT LoadTextureFromDDS(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* filename);

    std::shared_ptr<FTexture> GetTexture(const FWString& name) const;
private:
    inline static TMap<FWString, std::shared_ptr<FTexture>> textureMap;
#pragma endregion

#pragma region StaticMesh
public:
    static FStaticMeshRenderData* LoadStaticMeshAsset(const FString& PathFileName);

    static void CombineStaticMeshMaterialIndex(FStaticMeshRenderData& OutFStaticMesh);
    
    static void CombineSkeletalMeshMaterialIndex(FSkeletalMeshRenderData& OutSkeletalMesh);

    static bool SaveStaticMeshToBinary(const FWString& FilePath, const FStaticMeshRenderData& StaticMesh);

    static bool LoadStaticMeshFromBinary(const FWString& FilePath, FStaticMeshRenderData& OutStaticMesh);

    static UStaticMesh* CreateStaticMesh(const FString& filePath);
    
    static const TMap<FWString, UStaticMesh*>& GetStaticMeshes() { return StaticMeshMap; }

    static UStaticMesh* GetStaticMesh(FWString name);

    static int GetStaticMeshNum() { return StaticMeshMap.Num(); }

private:
    inline static TMap<FString, FStaticMeshRenderData*> ObjStaticMeshMap;

    inline static TMap<FWString, UStaticMesh*> StaticMeshMap;
#pragma endregion
    
#pragma region SkeletalMesh
public:

    static USkeletalMesh* LoadSkeletalMesh(const FString& FilePath);
    static FSkeletalMeshRenderData* LoadSkeletalMeshAsset(const FString& PathFileName);
    static USkeletalMesh* GetSkeletalMesh(const FWString& FilePath);

private:
    inline static TMap<FWString, USkeletalMesh*> SkeletalMeshMap;
    
#pragma endregion

#pragma region Material
public:
    static UMaterial* CreateMaterial(FObjMaterialInfo materialInfo);

    static TMap<FString, UMaterial*>& GetMaterials() { return MaterialMap; }

    static UMaterial* GetMaterial(FString name);

    static int GetMaterialNum() { return MaterialMap.Num(); }

private:
    inline static TMap<FString, UMaterial*> MaterialMap;
#pragma endregion
};
