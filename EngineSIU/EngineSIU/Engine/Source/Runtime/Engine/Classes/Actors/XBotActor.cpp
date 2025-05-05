#include "XBotActor.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"


AXBotActor::AXBotActor()
{
    SkeletalMeshComponent = AddComponent<USkeletalMeshComponent>(FName("SkeletalMeshComponent_0"));
    SkeletalMeshComponent->SetSkeletalMesh(FResourceManager::GetSkeletalMesh(L"Contents/Fbx/TheBoss.fbx"));
    RootComponent = SkeletalMeshComponent;
}

USkeletalMeshComponent* AXBotActor::GetShapeComponent() const
{
    return SkeletalMeshComponent;
}
