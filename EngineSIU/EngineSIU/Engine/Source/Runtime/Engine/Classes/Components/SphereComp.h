#pragma once
#include "Mesh/StaticMeshComponent.h"


class USphereComp : public UStaticMeshComponent
{
    DECLARE_CLASS(USphereComp, UStaticMeshComponent)

public:
    USphereComp();

    virtual void InitializeComponent() override;
    virtual void TickComponent(float DeltaTime) override;
};
