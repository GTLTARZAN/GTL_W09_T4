#pragma once
#include "Components/PrimitiveComponent.h"
#include "Components/ShapeComponent.h"

struct FOverlapResult;

class FCollisionManager;

using CollisionFunc = bool(FCollisionManager::*)(const UShapeComponent*, const UShapeComponent*, FOverlapResult&) const;

class FCollisionManager
{
public:
    FCollisionManager();
    ~FCollisionManager() = default;

    void CheckOverlap(const UWorld* World, const UPrimitiveComponent* Component, TArray<FOverlapResult>& OutOverlaps) const;

protected:
    bool IsOverlapped(const UPrimitiveComponent* Component, const UPrimitiveComponent* OtherComponent, FOverlapResult& OutResult) const;

    static constexpr SIZE_T NUM_TYPES = static_cast<SIZE_T>(EShapeType::MAX);
    
    CollisionFunc CollisionMatrix[NUM_TYPES + 1][NUM_TYPES + 1];

    bool Check_NotImplemented(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const;

    bool Check_Box_Box(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const;
    bool Check_Box_Sphere(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const;
    bool Check_Box_Capsule(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const;
    bool Check_Sphere_Box(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const;
    bool Check_Sphere_Sphere(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const;
    bool Check_Sphere_Capsule(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const;
    bool Check_Capsule_Box(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const;
    bool Check_Capsule_Sphere(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const;
    bool Check_Capsule_Capsule(const UShapeComponent* A, const UShapeComponent* B, FOverlapResult& OutResult) const;
};


