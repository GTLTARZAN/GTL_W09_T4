#include "FBXLoader.h"

#include <functional>

#include "Container/String.h"
#include "Define.h"
#include "FObjLoader.h"
#include "Asset/SkeletonAsset.h"
#include "Container/Map.h"
#include "Renderer/SkeletalMeshRenderPass.h"

FMatrix FFBXLoader::FbxAMatrixToFMatrix(const FbxAMatrix& Src)
{
    FMatrix RetMat;
    
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            RetMat.M[row][col] = static_cast<float>(Src.Get(row, col));

    return RetMat;
}

void FFBXLoader::ParseSkeleton(FSkeleton& OutSkeleton)
{
    std::vector<FbxNode*> BoneNodes;

    // 1. 본 노드 수집
    std::function<void(FbxNode*)> CollectBones = [&](FbxNode* node) {
        if (node->GetNodeAttribute() && node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
            BoneNodes.push_back(node);
        for (int i = 0; i < node->GetChildCount(); ++i)
            CollectBones(node->GetChild(i));
    };
    CollectBones(FbxLoadScene->GetRootNode());

    // 2. 본 정보 채우기
    for (size_t i = 0; i < BoneNodes.size(); ++i) {
        FbxNode* boneNode = BoneNodes[i];
        FBone bone;
        bone.BoneName = boneNode->GetName();

        // 부모 인덱스 찾기
        FbxNode* parent = boneNode->GetParent();
        bone.ParentIndex = 0xFFFF;
        for (size_t j = 0; j < BoneNodes.size(); ++j)
            if (BoneNodes[j] == parent)
                bone.ParentIndex = (uint16)j;

        // InvBindPose(역 바인드포즈) 구하기
        FbxAMatrix bindPose = boneNode->EvaluateGlobalTransform();
        bone.InvBindPose = FbxAMatrixToFMatrix(bindPose.Inverse().Transpose());

        OutSkeleton.Bones.Add(bone);
    }
}

void FFBXLoader::ParseMaterials(FbxNode* Node, TArray<FObjMaterialInfo>& OutMaterials)
{
    int MatCount = Node->GetMaterialCount();
    for (int i = 0; i < MatCount; ++i)
    {
        FbxSurfaceMaterial* Mat = Node->GetMaterial(i);
        if (!Mat) continue;

        FString MatName = FString(Mat->GetName());

        bool bIsFind = false;
        
        for (auto SkeletonMaterial: OutMaterials)
        {
            //TODO: ㅎㅐ쉬로 검사
            if (MatName == SkeletonMaterial.MaterialName)
            {
                //이미 있으면 continue
                bIsFind = true;
                break;
            }
        }

        if (bIsFind)
        {
            continue;
        }
        
        FObjMaterialInfo MatInfo;
        MatInfo.MaterialName = Mat->GetName();
        
        constexpr uint32 TexturesNum = static_cast<uint32>(EMaterialTextureSlots::MTS_MAX);
        MatInfo.TextureInfos.SetNum(TexturesNum);

        int Slot = 0;

        // Diffuse Color
        FbxProperty DiffuseProp = Mat->FindProperty(FbxSurfaceMaterial::sDiffuse);
        if (DiffuseProp.IsValid())
        {
            FbxDouble3 Diffuse = DiffuseProp.Get<FbxDouble3>();
            MatInfo.DiffuseColor = FVector(Diffuse[0], Diffuse[1], Diffuse[2]);
        }

        // Specular Color
        FbxProperty SpecularProp = Mat->FindProperty(FbxSurfaceMaterial::sSpecular);
        if (SpecularProp.IsValid())
        {
            FbxDouble3 Specular = SpecularProp.Get<FbxDouble3>();
            MatInfo.SpecularColor = FVector(Specular[0], Specular[1], Specular[2]);
        }

        // Shininess
        FbxProperty ShininessProp = Mat->FindProperty(FbxSurfaceMaterial::sShininess);
        if (ShininessProp.IsValid())
        {
            MatInfo.Shiness = (float)ShininessProp.Get<FbxDouble>();
        }

        // Diffuse Texture
        int DiffTexCount = DiffuseProp.GetSrcObjectCount<FbxTexture>();
        
        for (int t = 0; t < DiffTexCount; ++t)
        {
            FbxTexture* Tex = DiffuseProp.GetSrcObject<FbxTexture>(t);
            FbxFileTexture* FileTex = FbxCast<FbxFileTexture>(Tex);
            FWString FilePath = FString(FileTex->GetFileName()).ToWideString();
            if (FileTex && CreateTextureFromFile(FilePath, true))
            {
                MatInfo.TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Diffuse);
                Slot = static_cast<int>(EMaterialTextureSlots::MTS_Diffuse);
                MatInfo.TextureInfos[Slot].TexturePath = FilePath;
                break; // 여러개면 첫 번째만
            }
        }
        
        // Specular Texture
        int SpecularTexCount = SpecularProp.GetSrcObjectCount<FbxTexture>();
        
        for (int t = 0; t < SpecularTexCount; ++t)
        {
            FbxTexture* Tex = SpecularProp.GetSrcObject<FbxTexture>(t);
            FbxFileTexture* FileTex = FbxCast<FbxFileTexture>(Tex);
            FWString FilePath = FString(FileTex->GetFileName()).ToWideString();
            if (FileTex && CreateTextureFromFile(FilePath, true))
            {
                MatInfo.TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Specular);
                Slot = static_cast<int>(EMaterialTextureSlots::MTS_Specular);
                MatInfo.TextureInfos[Slot].TexturePath = FilePath;
                break; // 여러개면 첫 번째만
            }
        }
        
        // Normal Texture (Bump)
        FbxProperty BumpProp = Mat->FindProperty(FbxSurfaceMaterial::sNormalMap);
        if (!BumpProp.IsValid())
            BumpProp = Mat->FindProperty(FbxSurfaceMaterial::sBump);
        int NormTexCount = BumpProp.GetSrcObjectCount<FbxTexture>();
        for (int t = 0; t < NormTexCount; ++t)
        {
            FbxTexture* Tex = BumpProp.GetSrcObject<FbxTexture>(t);
            FbxFileTexture* FileTex = FbxCast<FbxFileTexture>(Tex);
            FWString FilePath = FString(FileTex->GetFileName()).ToWideString();
            if (FileTex && CreateTextureFromFile(FilePath, true))
            {
                MatInfo.TextureFlag |= static_cast<uint16>(EMaterialTextureFlags::MTF_Normal);
                Slot = static_cast<int>(EMaterialTextureSlots::MTS_Normal);
                MatInfo.TextureInfos[Slot].TexturePath = FilePath;
                break;
            }
        }

        OutMaterials.Add(MatInfo);
    }
}

bool FFBXLoader::CreateTextureFromFile(const FWString& Filename, bool bIsSRGB)
{
    if (FEngineLoop::ResourceManager.GetTexture(Filename))
    {
        return true;
    }

    HRESULT hr = FEngineLoop::ResourceManager.LoadTextureFromFile(FEngineLoop::GraphicDevice.Device, Filename.c_str(), bIsSRGB);

    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

void FFBXLoader::ParseMesh(FbxNode* Node, FSkeletalMeshRenderData& SkeletonData)
{
    FbxMesh* Mesh = Node->GetMesh();
    if (!Mesh) return;

    int VertexCount = Mesh->GetControlPointsCount();

    ParseMaterials(Node, SkeletonData.Materials);
    
    // 1. 정점 정보 추출 (공통)
    TArray<FSkinnedVertex> Vertices;
    Vertices.SetNum(VertexCount);

    int IndexStart = SkeletonData.Indices.Num();

    auto* uvElem = Mesh->GetElementUV();
    const char* uvSetName = uvElem ? uvElem->GetName() : nullptr;
    int PolyCount = Mesh->GetPolygonCount();

    FString SubMeshMaterial;
    
    int MaterialCount = Node->GetMaterialCount();
    for (int i=0;i<MaterialCount;i++)
    {
        FbxSurfaceMaterial* FbxMaterial = Node->GetMaterial(i);
        if (FbxMaterial)
        {
            SubMeshMaterial = FbxMaterial->GetName();
            break;
        }
    }
    
    int MatIndex = 0;
    FMaterialSubset MSubset{};

    for (int sm = 0; sm<SkeletonData.Materials.Num(); sm++)
    {
        //TODO: hash로 비교
        //머테리얼 리스트랑 지금 머테리얼 이름이랑 비교
        if (SkeletonData.Materials[sm].MaterialName == SubMeshMaterial)
        {
            MatIndex = sm;
            break;
        }
    }

    MSubset.MaterialIndex = MatIndex;
    MSubset.MaterialName = SkeletonData.Materials[MatIndex].MaterialName;
    
    for (int p=0; p<PolyCount; ++p)
    {
        // const int VertexInPolygon = Mesh->GetPolygonSize(p);

        for (int v=0;v<3;v++)
        {
            int cpIndex = Mesh->GetPolygonVertex(p, v);

            FbxVector4 Location = Mesh->GetControlPointAt(cpIndex);
            FbxVector4 Normal; Mesh->GetPolygonVertexNormal(p, v, Normal);
            
            FbxVector2 UV; bool UnMapped = false;
            if (uvSetName)
            {
                Mesh->GetPolygonVertexUV(p, v, uvSetName, UV, UnMapped);
            }

            FStaticMeshVertex OutV{};
            OutV.X = Location[0]; OutV.Y = Location[1]; OutV.Z = Location[2];
            OutV.NormalX = Normal[0]; OutV.NormalY = Normal[1]; OutV.NormalZ = Normal[2];
            OutV.U = UV[0]; OutV.V = 1 - UV[1];

            //TODO: Tangent계산
            OutV.TangentX = 0; OutV.TangentY = 0; OutV.TangentZ = 0; OutV.TangentW = 1;

            //필요없긴함
            OutV.R = 1; OutV.G = 1; OutV.B = 1; OutV.A = 1;
            
            OutV.MaterialIndex = MatIndex;
            
            std::string Key = std::to_string(OutV.X) + "/" + std::to_string(OutV.Y) + "/" + std::to_string(OutV.Z) + "/"
                            + std::to_string(OutV.NormalX) + "/" + std::to_string(OutV.NormalY) + "/" + std::to_string(OutV.NormalZ) + "/"
                            + std::to_string(OutV.TangentX) + "/" + std::to_string(OutV.TangentY) + "/" + std::to_string(OutV.TangentZ) + "/"
                            + std::to_string(OutV.U) + "/" + std::to_string(OutV.V) + "/" + std::to_string(OutV.MaterialIndex);

            int NewIdx = SkeletonData.Vertices.Num();
            
            if (VertexMap.Contains(Key))
            {
                //이미 있으면
                NewIdx = VertexMap[Key];
            }else
            {
                //중복이면
                SkeletonData.Vertices.Add(OutV);
            }
            
            SkeletonData.Indices.Add(NewIdx);
        }
    }

    // Tangent
    for (int32 i = 0; i < SkeletonData.Indices.Num(); i += 3)
    {
        FStaticMeshVertex& Vertex0 = SkeletonData.Vertices[SkeletonData.Indices[i]];
        FStaticMeshVertex& Vertex1 = SkeletonData.Vertices[SkeletonData.Indices[i + 1]];
        FStaticMeshVertex& Vertex2 = SkeletonData.Vertices[SkeletonData.Indices[i + 2]];

        FObjLoader::CalculateTangent(Vertex0, Vertex1, Vertex2);
        FObjLoader::CalculateTangent(Vertex1, Vertex2, Vertex0);
        FObjLoader::CalculateTangent(Vertex2, Vertex0, Vertex1);
    }

    MSubset.IndexStart = IndexStart;
    MSubset.IndexCount = SkeletonData.Indices.Num() - IndexStart;
    
    SkeletonData.MaterialSubsets.Add(MSubset);
}

void FFBXLoader::TraverseNodes(FbxNode* Node, FSkeletalMeshRenderData& SkeletalMeshData)
{
    if (!Node) return;
    auto* Attr = Node->GetNodeAttribute();
    if (Attr && Attr->GetAttributeType() == FbxNodeAttribute::eMesh) {
        ParseMesh(Node, SkeletalMeshData);
    }
    
    for (int i = 0; i < Node->GetChildCount(); ++i)
        TraverseNodes(Node->GetChild(i), SkeletalMeshData);
}

void FFBXLoader::ParseFBX(FSkeletalMeshRenderData& SkeletonaData)
{
    FbxGeometryConverter Converter(FbxLoadScene->GetFbxManager());
    Converter.Triangulate(FbxLoadScene.get(), true);
    
    VertexMap.Empty();
    
    // 1. 스켈레톤 파싱
    ParseSkeleton(SkeletonaData.Skeleton);

    // 2. 메시(스킨메시) 파싱
    TraverseNodes(FbxLoadScene->GetRootNode(), SkeletonaData);
}

void FFBXLoader::LoadSkeletalMesh(const FString& FilePath, FSkeletalMeshRenderData& OutSkeleton)
{
    FbxImporter* importer = FbxImporter::Create(FbxLoadManager.get(), "");
    if (!importer->Initialize(*FilePath, -1, FbxLoadManager->GetIOSettings())) {
        UE_LOG(LogLevel::Error, "%s Fbx Load Filed", *FilePath);
        return;
    }
    
    importer->Import(FbxLoadScene.get());
    
    ParseFBX(OutSkeleton);
}
