#pragma once
#include "MeshRenderPass.h"
#include "EngineBaseTypes.h"
#include "Container/Set.h"

#include "Define.h"
#include "Components/Light/PointLightComponent.h"

struct FStaticMeshRenderData;
class FShadowManager;
class FDXDShaderManager;
class UWorld;
class UMaterial;
class FEditorViewportClient;
class UStaticMeshComponent;
struct FStaticMaterial;
class FShadowRenderPass;

class FStaticMeshRenderPass : public FMeshRenderPassBase
{
public:
    FStaticMeshRenderPass();
    
    virtual ~FStaticMeshRenderPass();
    
    virtual void PrepareRenderArr() override;

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void ClearRenderArr() override;
    void RenderAllStaticMeshesForPointLight(const std::shared_ptr<FEditorViewportClient>& Viewport, UPointLightComponent*& PointLight);

    virtual void RenderAllStaticMeshes(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void RenderPrimitive(FStaticMeshRenderData* RenderData, TArray<FStaticMaterial*> Materials, TArray<UMaterial*> OverrideMaterials, int SelectedSubMeshIndex) const;
    
    void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices) const override;

    void RenderPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices) const override;

    // Shader 관련 함수 (생성/해제 등) // FMeshRenderPass에서 구현했습니다.
    
protected:


    TArray<UStaticMeshComponent*> StaticMeshComponents;

};
