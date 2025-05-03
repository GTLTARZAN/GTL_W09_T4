#pragma once
#include "Math/Matrix.h"

struct Bone
{
    FMatrix InvBindPose;
    FMatrix SkinningMatrix;
    const char* BoneName;
    uint8 ParentIndex; //루트일 경우 0xff
};

struct Skeleton
{
    uint32 NumBones;
    Bone* Bones;
};

struct BonePose
{
    FQuat Rotation;
    FVector Location;
    FVector Scale;
};

struct SkeletionPose
{
    Skeleton* Skeleton;
    BonePose* LocalPose;
    FMatrix GlobalPose;
};

struct AnimationSample
{
    BonePose* BonePoses;
};

struct AnimationClip
{
    Skeleton* Skeleton;
    float FramesPerSecond;
    uint32 FrameNum;
    AnimationClip* Samples;
    bool bIsLooping;
};

struct SkinnedVertex
{
    FVector Location;
    FVector Normal;
    FVector2D UV;
    uint8 BoneIndex[4];
    float BoneWeight[4];
};
