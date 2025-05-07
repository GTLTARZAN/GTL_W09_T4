#pragma once
#include "Define.h"
#include "Container/Array.h"
#include "Container/Set.h"
#include "Math/Matrix.h"
#include "Math/Quat.h"

struct FSkinnedVertex
{
    float X, Y, Z;    // Position
    float NormalX, NormalY, NormalZ;
    float TangentX, TangentY, TangentZ, TangentW;
    float U = 0, V = 0;
    uint32 MaterialIndex;
    uint32 BoneIndex[4] = {0,};
    float BoneWeight[4] = {0,};
};

struct FBonePose
{
    FMatrix LocalTransform = FMatrix::Identity;

    
    
    // FQuat Rotation = FQuat(0,0,0,0);
    // FVector Location = FVector::ZeroVector;
    // FVector Scale = FVector::OneVector;
};

struct FBone
{
    FMatrix InvBindPose;
    FBonePose Pose;
    FMatrix OffsetMatrix;
    // FMatrix SkinningMatrix;
    FString BoneName;
    uint16 ParentIndex = 0XFFFF; //루트일 경우 0xff
};

struct FSkeleton
{
    uint16 SkeletonIndex = 0;
    TArray<FBone> Bones;
};

struct FSkeletalPose
{
    FSkeleton* Skeleton;
    FBonePose* LocalPose;
    FMatrix GlobalPose;
};

struct FAnimationSample
{
    TArray<FBonePose> BonePoses;
};

struct FAnimationClip
{
    FSkeleton* Skeleton;
    float FramesPerSecond;
    uint32 FrameNum;
    TArray<FAnimationSample> Samples;
    bool bIsLooping;
};

struct FSkeletalMeshRenderData
{
    FWString ObjectName;
    FSkeleton Skeleton;
    
    //FStaticMeshVertex 와 Index 동기화 -> SkeletalMesh와 StaticMesh Shader를 합치면 한 버텍스에 둘 정보 넣어서 한번에 관리로 변경
    TArray<FSkinnedVertex> Vertices; 
    TArray<uint32> Indices;
    
    TArray<FSkeletalPose> Poses;

    TArray<FObjMaterialInfo> Materials;
    TArray<FMaterialSubset> MaterialSubsets;

    FVector BoundingBoxMin;
    FVector BoundingBoxMax;
};
