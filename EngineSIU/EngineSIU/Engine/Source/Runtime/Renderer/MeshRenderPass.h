#pragma once
#include "IRenderPass.h"
#include "EngineBaseTypes.h"
#include "Container/Set.h"

#include "Define.h"
#include "Components/Light/PointLightComponent.h"

class FShadowManager;
class FDXDShaderManager;
class UWorld;
class UMaterial;
class FEditorViewportClient;
class FShadowRenderPass;

class FMeshRenderPassBase : public IRenderPass
{
public:
    FMeshRenderPassBase();

    virtual ~FMeshRenderPassBase();

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;

    void InitializeShadowManager(class FShadowManager* InShadowManager);

    virtual void PrepareRenderArr() = 0;

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) = 0;

    virtual void ClearRenderArr() = 0;

    virtual void PrepareRenderState(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const;

    void UpdateLitUnlitConstant(int32 isLit) const;

    virtual void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices) const = 0;

    virtual void RenderPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices) const = 0;

    // Shader 관련 함수 (생성/해제 등)
    void CreateShader();
    void ReleaseShader();

    void ChangeViewMode(EViewModeIndex ViewMode);

protected:


    FDXDBufferManager* BufferManager;
    FGraphicsDevice* Graphics;
    FDXDShaderManager* ShaderManager;

    FShadowManager* ShadowManager;
};
