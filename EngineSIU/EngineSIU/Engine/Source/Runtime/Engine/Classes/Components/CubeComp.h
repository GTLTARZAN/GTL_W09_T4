#pragma once
#include "Mesh/StaticMeshComponent.h"

class UCubeComp : public UStaticMeshComponent
{
    DECLARE_CLASS(UCubeComp, UStaticMeshComponent)

public:
    UCubeComp();

    virtual void InitializeComponent() override;
    virtual void TickComponent(float DeltaTime) override;
};
