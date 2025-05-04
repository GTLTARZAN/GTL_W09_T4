#pragma once

#include "Core/Container/Array.h"
#include "Core/Math/Matrix.h"
#include "Core/Math/Vector.h"
#include "Core/Math/JungleMath.h"

/// <summary>
/// 관절의 정보를 나타내는 구조체
/// </summary>
struct FJoint
{
    // 관절의 기본 위치(Bind Pose)에 대한 역변환 행령 (Joint Space -> Model Space)
    FMatrix InvBindPose;
    // 사람이 읽이 편한 관절 이름
    FString Name;
    // 부모 관절의 인덱스
    uint8 ParentIndex;
};

/// <summary>
/// 스켈레톤의 정보를 나타내는 구조체
/// </summary>
struct FSkeleton
{
    // 스켈레톤에 포함 되어있는 관절들
    TArray<FJoint> Joints;
    // 총 관절의 수
    uint32 NumJoints;
};

/// <summary>
/// 관절의 회전, 위치, 스케일을 나타내는 구조체
/// </summary>
struct FJointPose
{
    FQuat Rotation;
    FVector Translation;
    float Scale;

	FMatrix ToMatrix() const
	{
		FVector ScaleVector = FVector(Scale, Scale, Scale);
		return JungleMath::CreateModelMatrix(Translation, Rotation, ScaleVector);
	}
};

/// <summary>
/// 전체 스켈레톤의 포즈를 나타내는 구조체
/// </summary>
struct FSkeletonPose
{
    // Joints 정보와 전체 Joint 수
    FSkeleton* Skeleton;
    // 관절의 로컬 포즈 (회전, 위치, 스케일)
    TArray<FJointPose> LocalJointPose;
    // 관절의 월드 포즈 (Joint Space -> Model Space)
    TArray<FMatrix> GlobalJointPose;

	// 관절의 월드 포즈를 계산하는 함수
	void ComputeGlobalPose()
	{
		GlobalJointPose.SetNum(Skeleton->NumJoints);
		for (uint32 i = 0; i < Skeleton->NumJoints; ++i)
		{
			const FJoint& Joint = Skeleton->Joints[i];
			if (Joint.ParentIndex == -1)
			{
				GlobalJointPose[i] = FMatrix::Identity;
			}
			else
			{
				GlobalJointPose[i] = GlobalJointPose[Joint.ParentIndex] * LocalJointPose[i].ToMatrix();
			}
		}
	}

	FMatrix GetSkinningMatrix(uint32 JointIndex) const
	{
		Skeleton->Joints[JointIndex].InvBindPose * GlobalJointPose[JointIndex];
	}

};

struct SkeletalVertex
{
    FVector Position;
    FVector Normal;
    FVector4 Tangent;
    FVector2D UV;
    int BoneIndices[4];
    float BoneWeights[3];
};

struct FBone
{
    FMatrix SkinningMatrix;
};
