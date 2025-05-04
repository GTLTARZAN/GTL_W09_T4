#pragma once
#include "Components/Mesh/StaticMeshComponent.h"


class UGizmoBaseComponent : public UStaticMeshComponent
{
    DECLARE_CLASS(UGizmoBaseComponent, UStaticMeshComponent)

public:
    enum EGizmoType : uint8
    {
        ArrowX,
        ArrowY,
        ArrowZ,
        CircleX,
        CircleY,
        CircleZ,
        ScaleX,
        ScaleY,
        ScaleZ
    };
    
public:
    UGizmoBaseComponent() = default;

    virtual void TickComponent(float DeltaTime) override;

private:
    EGizmoType GizmoType;

public:
    EGizmoType GetGizmoType() const { return GizmoType; }
    
    void SetGizmoType(EGizmoType InGizmoType) { GizmoType = InGizmoType; }

    float GizmoScale = 0.2f;
};
