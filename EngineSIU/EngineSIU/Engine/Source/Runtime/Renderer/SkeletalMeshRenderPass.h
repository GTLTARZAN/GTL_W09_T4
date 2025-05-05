#pragma once
#include "MeshRenderPass.h"
#include "Define.h"
#include "Components/Light/PointLightComponent.h"

struct FSkeletalMeshRenderData;

class FShadowManager;
class FDXDShaderManager;
class UWorld;
class UMaterial;
class FEditorViewportClient;
class FShadowRenderPass;
class USkeletalMeshComponent;

class FSkeletalMeshRenderPass : public FMeshRenderPassBase
{
public:
    FSkeletalMeshRenderPass();

    virtual ~FSkeletalMeshRenderPass();

    virtual void PrepareRenderArr() override;

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void ClearRenderArr() override;
    void RenderAllSkeletalMeshesForPointLight(const std::shared_ptr<FEditorViewportClient>& Viewport, UPointLightComponent*& PointLight);

    virtual void RenderAllSkeletalMeshes(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void RenderPrimitive(FSkeletalMeshRenderData* RenderData, TArray<FMaterial*> Materials, TArray<UMaterial*> OverrideMaterials, int SelectedSubMeshIndex) const;

    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices) const override;

    void RenderPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices) const override;

    // Shader 관련 함수 (생성/해제 등) // FMeshRenderPass에서 구현했습니다.

protected:

    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
};
