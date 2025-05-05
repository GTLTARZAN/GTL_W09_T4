#include "ResourceManager.h"
#include <fstream>
#include <ranges>
#include <wincodec.h>

#include "FBXLoader.h"
#include "Asset/SkeletonAsset.h"
#include "Components/SkySphereComponent.h"
#include "Components/Mesh/SkeletalMesh.h"
#include "D3D11RHI/GraphicDevice.h"
#include "DirectXTK/Include/DDSTextureLoader.h"
#include "Engine/FObjLoader.h"
#include "UObject/ObjectFactory.h"

#pragma region Texture
void FResourceManager::Initialize(FRenderer* renderer, FGraphicsDevice* device)
{
    LoadTextureFromDDS(device->Device, device->DeviceContext, L"Assets/Texture/font.dds");
    LoadTextureFromDDS(device->Device, device->DeviceContext, L"Assets/Texture/UUID_Font.dds");

    LoadTextureFromFile(device->Device, L"Assets/Texture/ocean_sky.jpg");
    LoadTextureFromFile(device->Device, L"Assets/Texture/font.png");
    LoadTextureFromFile(device->Device, L"Assets/Texture/emart.png");
    LoadTextureFromFile(device->Device, L"Assets/Texture/T_Explosion_SubUV.png");
    LoadTextureFromFile(device->Device, L"Assets/Texture/UUID_Font.png");
    LoadTextureFromFile(device->Device, L"Assets/Texture/Wooden Crate_Crate_BaseColor.png");
    LoadTextureFromFile(device->Device, L"Assets/Texture/spotLight.png");

    LoadTextureFromFile(device->Device, L"Assets/Editor/Icon/S_Actor.PNG");
    LoadTextureFromFile(device->Device, L"Assets/Editor/Icon/S_LightSpot.PNG");
    LoadTextureFromFile(device->Device, L"Assets/Editor/Icon/S_LightPoint.PNG");
    LoadTextureFromFile(device->Device, L"Assets/Editor/Icon/S_LightDirectional.PNG");
    LoadTextureFromFile(device->Device, L"Assets/Editor/Icon/S_ExpoHeightFog.PNG");
    LoadTextureFromFile(device->Device, L"Assets/Editor/Icon/S_AtmosphericHeightFog.PNG");
    LoadTextureFromFile(device->Device, L"Assets/Editor/Icon/AmbientLight_64x.png");
    
}

void FResourceManager::Release(FRenderer* renderer) {
    for (const auto& Pair : textureMap)
    {
        FTexture* texture = Pair.Value.get();
        texture->Release();
    }
    textureMap.Empty();
}

std::shared_ptr<FTexture> FResourceManager::GetTexture(const FWString& name) const
{
    auto* TempValue = textureMap.Find(name);
    return TempValue ? *TempValue : nullptr;
}

HRESULT FResourceManager::LoadTextureFromFile(ID3D11Device* device, const wchar_t* filename, bool bIsSRGB)
{
    IWICImagingFactory* wicFactory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;

    // WIC 팩토리 생성
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) return hr;

    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory));
    if (FAILED(hr)) return hr;


    // 이미지 파일 디코딩
    hr = wicFactory->CreateDecoderFromFilename(filename, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr)) return hr;


    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) return hr;

    // WIC 포맷 변환기 생성 (픽셀 포맷 변환)
    hr = wicFactory->CreateFormatConverter(&converter);
    if (FAILED(hr)) return hr;

    hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) return hr;

    // 이미지 크기 가져오기
    UINT width, height;
    frame->GetSize(&width, &height);

    // 픽셀 데이터 로드
    BYTE* imageData = new BYTE[width * height * 4];
    hr = converter->CopyPixels(nullptr, width * 4, width * height * 4, imageData);
    if (FAILED(hr)) {
        delete[] imageData;
        return hr;
    }

    // DirectX 11 텍스처 생성
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = bIsSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = imageData;
    initData.SysMemPitch = width * 4;
    ID3D11Texture2D* Texture2D;
    hr = device->CreateTexture2D(&textureDesc, &initData, &Texture2D);
    delete[] imageData;
    if (FAILED(hr)) return hr;

    // Shader Resource View 생성
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    ID3D11ShaderResourceView* TextureSRV;
    hr = device->CreateShaderResourceView(Texture2D, &srvDesc, &TextureSRV);

    // 리소스 해제
    wicFactory->Release();
    decoder->Release();
    frame->Release();
    converter->Release();

    //샘플러 스테이트 생성
    ID3D11SamplerState* SamplerState;
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    device->CreateSamplerState(&samplerDesc, &SamplerState);
    FWString name = FWString(filename);

    textureMap[name] = std::make_shared<FTexture>(TextureSRV, Texture2D, SamplerState, name, width, height);

    Console::GetInstance().AddLog(LogLevel::Warning, "Texture File Load Successs");
    return hr;
}

HRESULT FResourceManager::LoadTextureFromDDS(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* filename)
{

    ID3D11Resource* texture = nullptr;
    ID3D11ShaderResourceView* textureView = nullptr;

    HRESULT hr = DirectX::CreateDDSTextureFromFile(
        device, context,
        filename,
        &texture,
        &textureView
    );
    if (FAILED(hr) || texture == nullptr) abort();

#pragma region WidthHeight

    ID3D11Texture2D* texture2D = nullptr;
    hr = texture->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&texture2D);
    if (FAILED(hr) || texture2D == nullptr) {
        std::wcerr << L"Failed to query ID3D11Texture2D interface!" << std::endl;
        texture->Release();
        abort();
        return hr;
    }

    D3D11_TEXTURE2D_DESC texDesc;
    texture2D->GetDesc(&texDesc);
    uint32 width = texDesc.Width;
    uint32 height = texDesc.Height;

#pragma endregion WidthHeight

#pragma region Sampler
    ID3D11SamplerState* SamplerState;
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP; // WRAP -> CLAMP로 변경
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    device->CreateSamplerState(&samplerDesc, &SamplerState);
#pragma endregion Sampler

    FWString name = FWString(filename);

    textureMap[name] = std::make_shared<FTexture>(textureView, texture2D, SamplerState, name, width, height);

    Console::GetInstance().AddLog(LogLevel::Warning, "Texture File Load Successs");

    return hr;
}
#pragma endregion
#pragma region StaticMesh
FStaticMeshRenderData* FResourceManager::LoadStaticMeshAsset(const FString& PathFileName)
{
    FStaticMeshRenderData* NewStaticMesh = new FStaticMeshRenderData();

    if ( const auto It = ObjStaticMeshMap.Find(PathFileName))
    {
        return *It;
    }

    FWString BinaryPath = (PathFileName + ".bin").ToWideString();
    if (std::ifstream(BinaryPath).good())
    {
        if (LoadStaticMeshFromBinary(BinaryPath, *NewStaticMesh))
        {
            ObjStaticMeshMap.Add(PathFileName, NewStaticMesh);
            return NewStaticMesh;
        }
    }

    // Parse OBJ
    FObjInfo NewObjInfo;
    bool Result = FObjLoader::ParseOBJ(PathFileName, NewObjInfo);

    if (!Result)
    {
        delete NewStaticMesh;
        return nullptr;
    }

    // Material
    if (NewObjInfo.MaterialSubsets.Num() > 0)
    {
        Result = FObjLoader::ParseMaterial(NewObjInfo, *NewStaticMesh);

        if (!Result)
        {
            delete NewStaticMesh;
            return nullptr;
        }

        CombineStaticMeshMaterialIndex(*NewStaticMesh);

        for (int materialIndex = 0; materialIndex < NewStaticMesh->Materials.Num(); materialIndex++) {
            CreateMaterial(NewStaticMesh->Materials[materialIndex]);
        }
    }

    // Convert FStaticMeshRenderData
    Result = FObjLoader::ConvertToStaticMesh(NewObjInfo, *NewStaticMesh);
    if (!Result)
    {
        delete NewStaticMesh;
        return nullptr;
    }

    SaveStaticMeshToBinary(BinaryPath, *NewStaticMesh); 
    ObjStaticMeshMap.Add(PathFileName, NewStaticMesh);
    return NewStaticMesh;
}

void FResourceManager::CombineStaticMeshMaterialIndex(FStaticMeshRenderData& OutFStaticMesh)
{
    for (int32 i = 0; i < OutFStaticMesh.MaterialSubsets.Num(); i++)
    {
        FString MatName = OutFStaticMesh.MaterialSubsets[i].MaterialName;
        for (int32 j = 0; j < OutFStaticMesh.Materials.Num(); j++)
        {
            if (OutFStaticMesh.Materials[j].MaterialName == MatName)
            {
                OutFStaticMesh.MaterialSubsets[i].MaterialIndex = j;
                break;
            }
        }
    }
}

void FResourceManager::CombineSkeletalMeshMaterialIndex(FSkeletalMeshRenderData& OutSkeletalMesh)
{
    for (int32 i = 0; i < OutSkeletalMesh.MaterialSubsets.Num(); i++)
    {
        FString MatName = OutSkeletalMesh.MaterialSubsets[i].MaterialName;
        for (int32 j = 0; j < OutSkeletalMesh.Materials.Num(); j++)
        {
            if (OutSkeletalMesh.Materials[j].MaterialName == MatName)
            {
                OutSkeletalMesh.MaterialSubsets[i].MaterialIndex = j;
                break;
            }
        }
    }
}

bool FResourceManager::SaveStaticMeshToBinary(const FWString& FilePath, const FStaticMeshRenderData& StaticMesh)
{
    std::ofstream File(FilePath, std::ios::binary);
    if (!File.is_open())
    {
        assert("CAN'T SAVE STATIC MESH BINARY FILE");
        return false;
    }

    // Object Name
    Serializer::WriteFWString(File, StaticMesh.ObjectName);

    // Display Name
    Serializer::WriteFString(File, StaticMesh.DisplayName);

    // Vertices
    uint32 VertexCount = StaticMesh.Vertices.Num();
    File.write(reinterpret_cast<const char*>(&VertexCount), sizeof(VertexCount));
    File.write(reinterpret_cast<const char*>(StaticMesh.Vertices.GetData()), VertexCount * sizeof(FStaticMeshVertex));

    // Indices
    uint32 IndexCount = StaticMesh.Indices.Num();
    File.write(reinterpret_cast<const char*>(&IndexCount), sizeof(IndexCount));
    File.write(reinterpret_cast<const char*>(StaticMesh.Indices.GetData()), IndexCount * sizeof(UINT));

    // Materials
    uint32 MaterialCount = StaticMesh.Materials.Num();
    File.write(reinterpret_cast<const char*>(&MaterialCount), sizeof(MaterialCount));
    for (const FObjMaterialInfo& Material : StaticMesh.Materials)
    {
        Serializer::WriteFString(File, Material.MaterialName);
        
        File.write(reinterpret_cast<const char*>(&Material.TextureFlag), sizeof(Material.TextureFlag));
        
        File.write(reinterpret_cast<const char*>(&Material.bTransparent), sizeof(Material.bTransparent));
        File.write(reinterpret_cast<const char*>(&Material.DiffuseColor), sizeof(Material.DiffuseColor));
        File.write(reinterpret_cast<const char*>(&Material.SpecularColor), sizeof(Material.SpecularColor));
        File.write(reinterpret_cast<const char*>(&Material.AmbientColor), sizeof(Material.AmbientColor));
        File.write(reinterpret_cast<const char*>(&Material.EmissiveColor), sizeof(Material.EmissiveColor));
        
        File.write(reinterpret_cast<const char*>(&Material.Shiness), sizeof(Material.Shiness));
        File.write(reinterpret_cast<const char*>(&Material.IOR), sizeof(Material.IOR));
        File.write(reinterpret_cast<const char*>(&Material.Transparency), sizeof(Material.Transparency));
        File.write(reinterpret_cast<const char*>(&Material.BumpMultiplier), sizeof(Material.BumpMultiplier));
        File.write(reinterpret_cast<const char*>(&Material.IlluminanceModel), sizeof(Material.IlluminanceModel));

        File.write(reinterpret_cast<const char*>(&Material.Metallic), sizeof(Material.Metallic));
        File.write(reinterpret_cast<const char*>(&Material.Roughness), sizeof(Material.Roughness));

        for (uint8 i = 0; i < static_cast<uint8>(EMaterialTextureSlots::MTS_MAX); ++i)
        {
            Serializer::WriteFString(File, Material.TextureInfos[i].TextureName);
            Serializer::WriteFWString(File, Material.TextureInfos[i].TexturePath);
            File.write(reinterpret_cast<const char*>(&Material.TextureInfos[i].bIsSRGB), sizeof(Material.TextureInfos[i].bIsSRGB));
        }
    }

    // Material Subsets
    uint32 SubsetCount = StaticMesh.MaterialSubsets.Num();
    File.write(reinterpret_cast<const char*>(&SubsetCount), sizeof(SubsetCount));
    for (const FMaterialSubset& Subset : StaticMesh.MaterialSubsets)
    {
        Serializer::WriteFString(File, Subset.MaterialName);
        File.write(reinterpret_cast<const char*>(&Subset.IndexStart), sizeof(Subset.IndexStart));
        File.write(reinterpret_cast<const char*>(&Subset.IndexCount), sizeof(Subset.IndexCount));
        File.write(reinterpret_cast<const char*>(&Subset.MaterialIndex), sizeof(Subset.MaterialIndex));
    }

    // Bounding Box
    File.write(reinterpret_cast<const char*>(&StaticMesh.BoundingBoxMin), sizeof(FVector));
    File.write(reinterpret_cast<const char*>(&StaticMesh.BoundingBoxMax), sizeof(FVector));

    File.close();
    return true;
}

bool FResourceManager::LoadStaticMeshFromBinary(const FWString& FilePath, FStaticMeshRenderData& OutStaticMesh)
{
    std::ifstream File(FilePath, std::ios::binary);
    if (!File.is_open())
    {
        assert("CAN'T OPEN STATIC MESH BINARY FILE");
        return false;
    }

    TArray<TPair<FWString, bool>> Textures;

    // Object Name
    Serializer::ReadFWString(File, OutStaticMesh.ObjectName);

    //// Path Name
    //Serializer::ReadFWString(File, OutStaticMesh.PathName);

    // Display Name
    Serializer::ReadFString(File, OutStaticMesh.DisplayName);

    // Vertices
    uint32 VertexCount = 0;
    File.read(reinterpret_cast<char*>(&VertexCount), sizeof(VertexCount));
    OutStaticMesh.Vertices.SetNum(VertexCount);
    File.read(reinterpret_cast<char*>(OutStaticMesh.Vertices.GetData()), VertexCount * sizeof(FStaticMeshVertex));

    // Indices
    uint32 IndexCount = 0;
    File.read(reinterpret_cast<char*>(&IndexCount), sizeof(IndexCount));
    OutStaticMesh.Indices.SetNum(IndexCount);
    File.read(reinterpret_cast<char*>(OutStaticMesh.Indices.GetData()), IndexCount * sizeof(UINT));

    // Material
    uint32 MaterialCount = 0;
    File.read(reinterpret_cast<char*>(&MaterialCount), sizeof(MaterialCount));
    OutStaticMesh.Materials.SetNum(MaterialCount);
    for (FObjMaterialInfo& Material : OutStaticMesh.Materials)
    {
        Serializer::ReadFString(File, Material.MaterialName);
        File.read(reinterpret_cast<char*>(&Material.TextureFlag), sizeof(Material.TextureFlag));
        
        File.read(reinterpret_cast<char*>(&Material.bTransparent), sizeof(Material.bTransparent));
        File.read(reinterpret_cast<char*>(&Material.DiffuseColor), sizeof(Material.DiffuseColor));
        File.read(reinterpret_cast<char*>(&Material.SpecularColor), sizeof(Material.SpecularColor));
        File.read(reinterpret_cast<char*>(&Material.AmbientColor), sizeof(Material.AmbientColor));
        File.read(reinterpret_cast<char*>(&Material.EmissiveColor), sizeof(Material.EmissiveColor));
        
        File.read(reinterpret_cast<char*>(&Material.Shiness), sizeof(Material.Shiness));
        File.read(reinterpret_cast<char*>(&Material.IOR), sizeof(Material.IOR));
        File.read(reinterpret_cast<char*>(&Material.Transparency), sizeof(Material.Transparency));
        File.read(reinterpret_cast<char*>(&Material.BumpMultiplier), sizeof(Material.BumpMultiplier));
        File.read(reinterpret_cast<char*>(&Material.IlluminanceModel), sizeof(Material.IlluminanceModel));

        File.read(reinterpret_cast<char*>(&Material.Metallic), sizeof(Material.Metallic));
        File.read(reinterpret_cast<char*>(&Material.Roughness), sizeof(Material.Roughness));

        uint8 TextureNum = static_cast<uint8>(EMaterialTextureSlots::MTS_MAX);
        Material.TextureInfos.SetNum(TextureNum);
        for (uint8 i = 0; i < TextureNum; ++i)
        {
            Serializer::ReadFString(File, Material.TextureInfos[i].TextureName);
            Serializer::ReadFWString(File, Material.TextureInfos[i].TexturePath);
            File.read(reinterpret_cast<char*>(&Material.TextureInfos[i].bIsSRGB), sizeof(Material.TextureInfos[i].bIsSRGB));

            Textures.AddUnique({Material.TextureInfos[i].TexturePath, Material.TextureInfos[i].bIsSRGB});
        }
    }

    // Material Subset
    uint32 SubsetCount = 0;
    File.read(reinterpret_cast<char*>(&SubsetCount), sizeof(SubsetCount));
    OutStaticMesh.MaterialSubsets.SetNum(SubsetCount);
    for (FMaterialSubset& Subset : OutStaticMesh.MaterialSubsets)
    {
        Serializer::ReadFString(File, Subset.MaterialName);
        File.read(reinterpret_cast<char*>(&Subset.IndexStart), sizeof(Subset.IndexStart));
        File.read(reinterpret_cast<char*>(&Subset.IndexCount), sizeof(Subset.IndexCount));
        File.read(reinterpret_cast<char*>(&Subset.MaterialIndex), sizeof(Subset.MaterialIndex));
    }

    // Bounding Box
    File.read(reinterpret_cast<char*>(&OutStaticMesh.BoundingBoxMin), sizeof(FVector));
    File.read(reinterpret_cast<char*>(&OutStaticMesh.BoundingBoxMax), sizeof(FVector));

    File.close();

    // Texture Load
    if (Textures.Num() > 0)
    {
        for (const TPair<FWString, bool>& Texture : Textures)
        {
            if (FEngineLoop::ResourceManager.GetTexture(Texture.Key) == nullptr)
            {
                FEngineLoop::ResourceManager.LoadTextureFromFile(FEngineLoop::GraphicDevice.Device, Texture.Key.c_str(), Texture.Value);
            }
        }
    }

    return true;
}

UStaticMesh* FResourceManager::GetStaticMesh(FWString name)
{
    return StaticMeshMap[name];
}

UStaticMesh* FResourceManager::CreateStaticMesh(const FString& filePath)
{
    FStaticMeshRenderData* StaticMeshRenderData = LoadStaticMeshAsset(filePath);

    if (StaticMeshRenderData == nullptr) return nullptr;

    UStaticMesh* StaticMesh = GetStaticMesh(StaticMeshRenderData->ObjectName);
    if (StaticMesh != nullptr)
    {
        return StaticMesh;
    }

    StaticMesh = FObjectFactory::ConstructObject<UStaticMesh>(nullptr);
    StaticMesh->SetData(StaticMeshRenderData);

    StaticMeshMap.Add(StaticMeshRenderData->ObjectName, StaticMesh); 
    return StaticMesh;
}
#pragma endregion

#pragma region SkeletalMesh

USkeletalMesh* FResourceManager::LoadSkeletalMesh(const FString& FilePath)
{
    FWString WideFilePath = FilePath.ToWideString();

    if (SkeletalMeshMap.Contains(WideFilePath))
    {
        return SkeletalMeshMap[WideFilePath];
    }
    
    FSkeletalMeshRenderData* SkeletalMeshData = LoadSkeletalMeshAsset(FilePath);

    if (SkeletalMeshData == nullptr) return nullptr;
    
    SkeletalMeshData->ObjectName = WideFilePath;

    USkeletalMesh* SkeletalMesh = GetSkeletalMesh(SkeletalMeshData->ObjectName);
    if (SkeletalMesh != nullptr)
    {
        return SkeletalMesh;
    }

    SkeletalMesh = FObjectFactory::ConstructObject<USkeletalMesh>(nullptr);
    SkeletalMesh->SetData(SkeletalMeshData);

    SkeletalMeshMap.Add(WideFilePath, SkeletalMesh);

    return SkeletalMesh;
}

FSkeletalMeshRenderData* FResourceManager::LoadSkeletalMeshAsset(const FString& PathFileName)
{
    // FWString BinaryPath = (PathFileName + ".bin").ToWideString();
    // if (std::ifstream(BinaryPath).good())
    // {
    //     if (LoadStaticMeshFromBinary(BinaryPath, *NewStaticMesh))
    //     {
    //         ObjStaticMeshMap.Add(PathFileName, NewStaticMesh);
    //         return NewStaticMesh;
    //     }
    // }

    FSkeletalMeshRenderData* NewSkeletalMesh = new FSkeletalMeshRenderData();

    FFBXLoader::LoadSkeletalMesh(PathFileName, *NewSkeletalMesh);
    
    // Material

    for (auto Material : NewSkeletalMesh->Materials) {
        CreateMaterial(Material);
    }
    
    // SaveStaticMeshToBinary(BinaryPath, *NewStaticMesh);
    
    return NewSkeletalMesh;
}

USkeletalMesh* FResourceManager::GetSkeletalMesh(const FWString& FilePath)
{
    return SkeletalMeshMap[FilePath];
}

#pragma endregion
#pragma region Material

UMaterial* FResourceManager::CreateMaterial(FObjMaterialInfo materialInfo)
{
    if (MaterialMap[materialInfo.MaterialName] != nullptr)
        return MaterialMap[materialInfo.MaterialName];

    UMaterial* newMaterial = FObjectFactory::ConstructObject<UMaterial>(nullptr); // Material은 Outer가 없이 따로 관리되는 객체이므로 Outer가 없음으로 설정. 추후 Garbage Collection이 추가되면 AssetManager를 생성해서 관리.
    newMaterial->SetMaterialInfo(materialInfo);
    MaterialMap.Add(materialInfo.MaterialName, newMaterial);
    return newMaterial;
}

UMaterial* FResourceManager::GetMaterial(FString name)
{
    return MaterialMap[name];
}
#pragma endregion
