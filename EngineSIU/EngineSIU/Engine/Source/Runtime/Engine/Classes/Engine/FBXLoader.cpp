#include "FBXLoader.h"

#include <functional>

#include "Container/String.h"
#include "Define.h"
#include "Asset/SkeletonAsset.h"
#include "Asset/StaticMeshAsset.h"
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

    // 1. 정점 정보 추출 (공통)
    TArray<FStaticMeshVertex> Vertices;
    Vertices.SetNum(VertexCount);

    auto* Normals = Mesh->GetElementNormal(0);
    auto* UVs = Mesh->GetElementUV(0);
    auto* Tangents = Mesh->GetElementTangent(0);
    auto* BiNormal = Mesh->GetElementBinormal(0);
    
    for (int i = 0; i < VertexCount; ++i) {
        FStaticMeshVertex& V = Vertices[i];
        FbxVector4 P = Mesh->GetControlPointAt(i);
        V.X = P[0]; V.Y = P[1]; V.Z = P[2];

        if (Normals)
        {
            FbxVector4 N = Normals->GetDirectArray().GetAt(i);
            V.NormalX = N[0]; V.NormalY = N[1]; V.NormalZ = N[2];
        }

        if (Tangents)
        {
            FbxVector4 T = Tangents->GetDirectArray().GetAt(i);
            V.TangentX = T[0]; V.TangentY = T[1]; V.TangentZ = T[2];
        }

        if (BiNormal)
        {
            FbxVector4 B = BiNormal->GetDirectArray().GetAt(i);
            FVector BiTangent = FVector(B[0], B[1], B[2]).GetSafeNormal();
            
            const float Sign = (FVector::DotProduct(FVector::CrossProduct(FVector(V.NormalX, V.NormalY, V.NormalZ), FVector(V.TangentX, V.TangentY, V.TangentZ)), BiTangent) < 0.f) ? -1.f : 1.f;
            V.TangentW = Sign;
        }
        
        if (UVs) {
            FbxVector2 UV = UVs->GetDirectArray().GetAt(i);
            V.U = UV[0]; V.V = 1 - UV[1];
        }
    }

    TArray<FSkinnedVertex> SkinningData;
    SkinningData.SetNum(VertexCount);
    // 스킨(본 가중치) 추출
    int DeformerCount = Mesh->GetDeformerCount(FbxDeformer::eSkin);
    if (DeformerCount > 0) {
        FbxSkin* Skin = static_cast<FbxSkin*>(Mesh->GetDeformer(0, FbxDeformer::eSkin));
        int BoneCount = Skin->GetClusterCount();
        for (int c = 0; c < BoneCount; ++c) {
            //Cluster가 Bone임
            FbxCluster* Bone = Skin->GetCluster(c);
            int BoneIdx = -1;
            FString BoneName = Bone->GetLink()->GetName();
            for (size_t b = 0; b < SkeletonData.Skeleton.Bones.Num(); ++b)
                if (SkeletonData.Skeleton.Bones[b].BoneName == BoneName)
                    BoneIdx = (int)b;
            if (BoneIdx < 0) continue;

            int* Indices = Bone->GetControlPointIndices();
            double* Weights = Bone->GetControlPointWeights();
            int Count = Bone->GetControlPointIndicesCount();
            for (int k = 0; k < Count; ++k) {
                int VertIdx = Indices[k];
                float Weight = (float)Weights[k];
                // 가장 가중치가 낮은 슬롯에 할당
                FSkinnedVertex& V = SkinningData[VertIdx];
                for (int s = 0; s < 4; ++s) {
                    if (V.BoneWeight[s] < 1e-4f) {
                        //어떤 인덱스에 얼마만큼의 가중치를 줄지 저장
                        V.BoneIndex[s] = BoneIdx;
                        V.BoneWeight[s] = Weight;
                        break;
                    }
                }
            }
        }
    }
    
    int PolyCount = Mesh->GetPolygonCount();
    FbxLayerElementMaterial* MatElement = Mesh->GetElementMaterial();
    
    // 머티리얼별로 임시 인덱스 버퍼
    TMap<int, TArray<uint32>> MaterialToIndices;

    // 1. 폴리곤별로 머티리얼 인덱스에 따라 인덱스 모으기
    for (int i = 0; i < PolyCount; ++i) {
        int MatIdx = 0;
        if (MatElement && MatElement->GetMappingMode() == FbxLayerElement::eByPolygon)
            MatIdx = MatElement->GetIndexArray().GetAt(i);
        
        int PolySize = Mesh->GetPolygonSize(i);
        
        for (int j = 0; j < PolySize; ++j) {
            int GlobalIdx = Mesh->GetPolygonVertex(i, j);
            MaterialToIndices.FindOrAdd(MatIdx).Add(GlobalIdx);
        }
    }

    uint32 IndexOffset = SkeletonData.Vertices.Num();
    // 2. 전체 Vertices/Indices 배열에 저장, MaterialSubset 정보 기록
    SkeletonData.Vertices.Append(Vertices); // 컨트롤포인트 기반이므로 그대로 복사

    for (auto& Pair : MaterialToIndices) {
        int MatIdx = Pair.Key;
        TArray<uint32>& SubIndices = Pair.Value;

        for (auto& index : SubIndices)
        {
            index += IndexOffset;
        }
        
        FMaterialSubset Subset;
        Subset.IndexStart = SkeletonData.Indices.Num();
        Subset.IndexCount = SubIndices.Num();
        
        // MaterialName 할당
        FbxSurfaceMaterial* Mat = Node->GetMaterial(MatIdx);
        FString MatName = FString(Mat->GetName());
        if (Mat)
        {
            Subset.MaterialName = MatName;
        }

        bool bIsFind = false;
        
        for (auto SkeletonSubset : SkeletonData.MaterialSubsets)
        {
            //Todo: hash로 비교
            if (MatName == SkeletonSubset.MaterialName)
            {
                Subset.MaterialIndex = SkeletonSubset.MaterialIndex;
                bIsFind = true;
                break;
            }
        }

        if (bIsFind == false)
        {
            Subset.MaterialIndex = GlobalSubsetMaterialIndex++;
        }

        SkeletonData.Indices.Append(SubIndices);
        SkeletonData.MaterialSubsets.Add(Subset);
    }

    ParseMaterials(Node, SkeletonData.Materials);
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
    GlobalSubsetMaterialIndex = 0;
    
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
