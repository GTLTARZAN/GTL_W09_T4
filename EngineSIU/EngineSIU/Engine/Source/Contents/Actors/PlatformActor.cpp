
#include "PlatformActor.h"

#include "Engine/FObjLoader.h"

APlatformActor::APlatformActor()
{
    BoxComponent = AddComponent<UBoxComponent>(FName("BoxComponent_0"));
    RootComponent = BoxComponent;

    MeshComponent = AddComponent<UStaticMeshComponent>(FName("MeshComponent_0"));
    MeshComponent->SetStaticMesh(FResourceManager::GetStaticMesh(L"Contents/Oil/Oil.obj"));
    MeshComponent->SetupAttachment(BoxComponent);
}
