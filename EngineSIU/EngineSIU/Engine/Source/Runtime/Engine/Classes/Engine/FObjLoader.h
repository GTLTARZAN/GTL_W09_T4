#pragma once

#include "EngineLoop.h"
#include "HAL/PlatformType.h"
#include "Serialization/Serializer.h"

class UStaticMesh;
class FResourceManager;

struct FStaticMeshVertex;
struct FStaticMeshRenderData;

struct FObjLoader
{
    // Obj Parsing (*.obj to FObjInfo)
    static bool ParseOBJ(const FString& ObjFilePath, FObjInfo& OutObjInfo);

    // Material Parsing (*.obj to MaterialInfo)
    static bool ParseMaterial(FObjInfo& OutObjInfo, FStaticMeshRenderData& OutFStaticMesh);

    // Convert the Raw data to Cooked data (FStaticMeshRenderData)
    static bool ConvertToStaticMesh(const FObjInfo& RawData, FStaticMeshRenderData& OutStaticMesh);

    static bool CreateTextureFromFile(const FWString& Filename, bool bIsSRGB = true);

    static void ComputeBoundingBox(const TArray<FStaticMeshVertex>& InVertices, FVector& OutMinVector, FVector& OutMaxVector);

private:
    static void CalculateTangent(FStaticMeshVertex& PivotVertex, const FStaticMeshVertex& Vertex1, const FStaticMeshVertex& Vertex2);
};
