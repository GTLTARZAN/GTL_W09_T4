#include "USkinnedMeshComponent.h"
#include "UObject/Casts.h"

UObject* USkinnedMeshComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewComponent->OverrideMaterials = OverrideMaterials;
    return NewComponent;
}

void USkinnedMeshComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
}

void USkinnedMeshComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
}
