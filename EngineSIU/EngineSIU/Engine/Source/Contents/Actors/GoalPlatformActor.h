#pragma once
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Components/Mesh/StaticMeshComponent.h"

class AGoalPlatformActor : public AActor
{
    DECLARE_CLASS(AGoalPlatformActor, AActor)

public:
    AGoalPlatformActor();
    virtual ~AGoalPlatformActor() override = default;

protected:
    UPROPERTY
    (UBoxComponent*, BoxComponent, = nullptr)

    UPROPERTY
    (UStaticMeshComponent*, MeshComponent, = nullptr)
};
