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
        {
            if (BoneNodes[j] == parent)
            {
                bone.ParentIndex = (uint16)j;
            }
        }
        
        FbxAMatrix BindPose = boneNode->EvaluateGlobalTransform();
        // InvBindPose(역 바인드포즈) 구하기
        FbxAMatrix InvBindPose = BindPose.Inverse();
        bone.InvBindPose = FbxAMatrixToFMatrix(InvBindPose);

        FBonePose BonePose;
        
        //처음에는 글로벌, 루트는 글로벌이 곧 로컬
        BonePose.LocalTransform = FbxAMatrixToFMatrix(BindPose);
        if (bone.ParentIndex != 0xFFFF)
        {
            BonePose.LocalTransform = OutSkeleton.Bones[bone.ParentIndex].InvBindPose * BonePose.LocalTransform;
        }
        bone.Pose = BonePose;
        
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
            if (MatName.GetTypeHash() == SkeletonMaterial.MaterialName.GetTypeHash())
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

    ParseMaterials(Node, SkeletonData.Materials);

    int IndexStart = SkeletonData.Indices.Num();

    auto* uvElem = Mesh->GetElementUV();
    const char* uvSetName = uvElem ? uvElem->GetName() : nullptr;
    int PolyCount = Mesh->GetPolygonCount();
    
    int cpCount = Mesh->GetControlPointsCount();
    std::vector<std::vector<std::pair<int, double>>> cpWeights(cpCount);
    for (int si = 0; si < Mesh->GetDeformerCount(FbxDeformer::eSkin); ++si)
    {
        FbxSkin* skin = static_cast<FbxSkin*>(Mesh->GetDeformer(si, FbxDeformer::eSkin));
        for (int ci = 0; ci < skin->GetClusterCount(); ++ci)
        {
            FbxCluster* cluster = skin->GetCluster(ci);
            FbxNode* boneNode = cluster->GetLink();
            if (!boneNode) continue;
            
            FString boneName = FString(boneNode->GetName());
            int boneIndex = INDEX_NONE;
            for (int b = 0; b < SkeletonData.Skeleton.Bones.Num(); ++b)
            {
                if (SkeletonData.Skeleton.Bones[b].BoneName.GetTypeHash() == boneName.GetTypeHash())
                {
                    // FbxAMatrix OffsetTransformMatrix;
                    // cluster->GetTransformMatrix(OffsetTransformMatrix);
                    // SkeletonData.Skeleton.Bones[b].OffsetMatrix = FbxAMatrixToFMatrix(OffsetTransformMatrix);
                    
                    boneIndex = b;
                    break;
                }
            }
            
            if (boneIndex < 0)  continue;
            
            int* cpIdxArr = cluster->GetControlPointIndices();
            double* wArr = cluster->GetControlPointWeights();
            int     cnt = cluster->GetControlPointIndicesCount();
            for (int k = 0; k < cnt; ++k)
            {
                int cpIdx = cpIdxArr[k];
                double w = wArr[k];
                if (cpIdx >= 0 && cpIdx < cpCount && w > 0.0)
                    cpWeights[cpIdx].emplace_back(boneIndex, w);
            }
        }
    }
    
    FString SubMeshMaterial;

    //현재 서브메쉬가 사용하는 Material가져오기 -> 서브메쉬 별로 들어오는데 Material은 왜 배열로 있지 node의 범용성떄문인듯?
    int MaterialCount = Node->GetMaterialCount();
    for (int i=0;i<MaterialCount;i++)
    {
        FbxSurfaceMaterial* FbxMaterial = Node->GetMaterial(i);
        if (FbxMaterial)
        {
            SubMeshMaterial = FbxMaterial->GetName();
            int MatIndex = 0;
            FMaterialSubset MSubset{};

            //무슨 material 사용하는지 찾기
            for (int sm = 0; sm<SkeletonData.Materials.Num(); sm++)
            {
                //머테리얼 리스트랑 지금 머테리얼 이름이랑 비교
                if (SkeletonData.Materials[sm].MaterialName.GetTypeHash() == SubMeshMaterial.GetTypeHash())
                {
                    MatIndex = sm;
                    break;
                }
            }
            
            for (int p=0; p<PolyCount; ++p)
            {
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

                    FSkinnedVertex OutV{};
                    OutV.X = Location[0]; OutV.Y = Location[1]; OutV.Z = Location[2];
                    OutV.NormalX = Normal[0]; OutV.NormalY = Normal[1]; OutV.NormalZ = Normal[2];
                    OutV.U = UV[0]; OutV.V = 1 - UV[1];

                    //TODO: Tangent계산
                    OutV.TangentX = 0; OutV.TangentY = 0; OutV.TangentZ = 0; OutV.TangentW = 1;
                    
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
                        // --- FVertexSkeletal (CPU 스키닝용) ---
                        auto& wlist = cpWeights[cpIndex];
                        std::sort(wlist.begin(), wlist.end(),
                            [](auto& A, auto& B) { return A.second > B.second; });
                        float totalW = 0.f;
                        int useN = FMath::Min((int)wlist.size(), 4);
                        for (int k = 0; k < useN; ++k)
                        {
                            OutV.BoneIndex[k] = wlist[k].first;
                            OutV.BoneWeight[k] = (float)wlist[k].second;
                            totalW += OutV.BoneWeight[k];
                        }
                        for (int k = useN; k < 4; ++k)
                        {
                            OutV.BoneIndex[k] = INDEX_NONE;
                            OutV.BoneWeight[k] = 0.f;
                        }
                        if (totalW > 0.f)
                            for (int k = 0; k < useN; ++k)
                                OutV.BoneWeight[k] /= totalW;

                        SkeletonData.Vertices.Add(OutV);
                    }
                    
                    SkeletonData.Indices.Add(NewIdx);
                }
            }

            MSubset.IndexStart = IndexStart;
            MSubset.IndexCount = SkeletonData.Indices.Num() - IndexStart;
            MSubset.MaterialName = SkeletonData.Materials[MatIndex].MaterialName;

            // Tangent
            for (int32 i = 0; i < SkeletonData.Indices.Num(); i += 3)
            {
                FSkinnedVertex& Vertex0 = SkeletonData.Vertices[SkeletonData.Indices[i]];
                FSkinnedVertex& Vertex1 = SkeletonData.Vertices[SkeletonData.Indices[i + 1]];
                FSkinnedVertex& Vertex2 = SkeletonData.Vertices[SkeletonData.Indices[i + 2]];

                //TODO: 따로 Math로 CalculateTangent빼서 ObjLoader전부 안부르게 변경
                //혹은 fbx가 갖고있는 tangent값 불러오게 변경
                CalculateTangent(Vertex0, Vertex1, Vertex2);
                CalculateTangent(Vertex1, Vertex2, Vertex0);
                CalculateTangent(Vertex2, Vertex0, Vertex1);
            }
            
            SkeletonData.MaterialSubsets.Add(MSubset);
        }
    }
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

void FFBXLoader::CalculateTangent(FSkinnedVertex& PivotVertex, const FSkinnedVertex& Vertex1, const FSkinnedVertex& Vertex2)
{
    const float s1 = Vertex1.U - PivotVertex.U;
    const float t1 = Vertex1.V - PivotVertex.V;
    const float s2 = Vertex2.U - PivotVertex.U;
    const float t2 = Vertex2.V - PivotVertex.V;
    const float E1x = Vertex1.X - PivotVertex.X;
    const float E1y = Vertex1.Y - PivotVertex.Y;
    const float E1z = Vertex1.Z - PivotVertex.Z;
    const float E2x = Vertex2.X - PivotVertex.X;
    const float E2y = Vertex2.Y - PivotVertex.Y;
    const float E2z = Vertex2.Z - PivotVertex.Z;

    const float Denominator = s1 * t2 - s2 * t1;
    FVector Tangent(1, 0, 0);
    FVector BiTangent(0, 1, 0);
    FVector Normal(PivotVertex.NormalX, PivotVertex.NormalY, PivotVertex.NormalZ);
    
    if (FMath::Abs(Denominator) > SMALL_NUMBER)
    {
        // 정상적인 계산 진행
        const float f = 1.f / Denominator;
        
        const float Tx = f * (t2 * E1x - t1 * E2x);
        const float Ty = f * (t2 * E1y - t1 * E2y);
        const float Tz = f * (t2 * E1z - t1 * E2z);
        Tangent = FVector(Tx, Ty, Tz).GetSafeNormal();

        const float Bx = f * (-s2 * E1x + s1 * E2x);
        const float By = f * (-s2 * E1y + s1 * E2y);
        const float Bz = f * (-s2 * E1z + s1 * E2z);
        BiTangent = FVector(Bx, By, Bz).GetSafeNormal();
    }
    else
    {
        // 대체 탄젠트 계산 방법
        // 방법 1: 다른 방향에서 탄젠트 계산 시도
        FVector Edge1(E1x, E1y, E1z);
        FVector Edge2(E2x, E2y, E2z);
    
        // 기하학적 접근: 두 에지 사이의 각도 이등분선 사용
        Tangent = (Edge1.GetSafeNormal() + Edge2.GetSafeNormal()).GetSafeNormal();
    
        // 만약 두 에지가 평행하거나 반대 방향이면 다른 방법 사용
        if (Tangent.IsNearlyZero())
        {
            // TODO: 기본 축 방향 중 하나 선택 (메시의 주 방향에 따라 선택)
            Tangent = FVector(1.0f, 0.0f, 0.0f);
        }
    }

    Tangent = (Tangent - Normal * FVector::DotProduct(Normal, Tangent)).GetSafeNormal();
    
    const float Sign = (FVector::DotProduct(FVector::CrossProduct(Normal, Tangent), BiTangent) < 0.f) ? -1.f : 1.f;

    PivotVertex.TangentX = Tangent.X;
    PivotVertex.TangentY = Tangent.Y;
    PivotVertex.TangentZ = Tangent.Z;
    PivotVertex.TangentW = Sign;
}
