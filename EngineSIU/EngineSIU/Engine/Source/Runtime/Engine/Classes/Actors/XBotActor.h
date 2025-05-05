#pragma once
#include "GameFramework/Actor.h"

class USphereComponent;
class USkeletalMeshComponent;

class AXBotActor : public AActor
{
    DECLARE_CLASS(AXBotActor, AActor)
public:
    AXBotActor();
    USkeletalMeshComponent* GetShapeComponent() const;

protected:
    UPROPERTY
    (USkeletalMeshComponent*, SkeletalMeshComponent, = nullptr)
    // (USphereComponent*, SphereComponent, = nullptr);

};

