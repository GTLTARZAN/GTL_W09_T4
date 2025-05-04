#include "TriggerBox.h"
#include "Components/Mesh/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/FObjLoader.h"

ATriggerBox::ATriggerBox()
{
    BoxComponent = AddComponent<UBoxComponent>(FName("BoxComponent_0"));
    RootComponent = BoxComponent;

    MeshComponent = AddComponent<UStaticMeshComponent>(FName("MeshComponent_0"));
    MeshComponent->SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/FryBasket/FryBasket.obj"));
    MeshComponent->SetupAttachment(BoxComponent);
}

void ATriggerBox::PostSpawnInitialize()
{
    AActor::PostSpawnInitialize();
}

void ATriggerBox::BeginPlay()
{
    AActor::BeginPlay();
}

void ATriggerBox::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);
}

void ATriggerBox::ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
}
