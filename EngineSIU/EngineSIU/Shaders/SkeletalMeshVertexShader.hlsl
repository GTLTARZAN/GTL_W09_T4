
#include "ShaderRegisters.hlsl"

#ifdef LIGHTING_MODEL_GOURAUD
SamplerState DiffuseSampler : register(s0);

Texture2D DiffuseTexture : register(t0);

cbuffer MaterialConstants : register(b1)
{
    FMaterial Material;
}

#include "Light.hlsl"
#endif

cbuffer BoneInfo : register(b2){
    row_major matrix SkinningMatrix[512];
}

struct VS_INPUT_SkeletalMesh
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 UV : TEXCOORD;
    uint MaterialIndex : MATERIAL_INDEX;
    uint4 BoneIndex : BONE_INDEX;
    float4 BoneWeight : BONE_WEIGHT;
};

PS_INPUT_StaticMesh mainVS(VS_INPUT_SkeletalMesh Input)
{
    PS_INPUT_StaticMesh Output;

    float4 ModelPos = float4(Input.Position, 1.0);
    // Output.Position = ModelPos;

    //Skinning곱해주기
    float4 skinnedPos = float4(0,0,0,0);

    for (int i = 0; i < 4; ++i)
    {
        if (Input.BoneIndex[i] == 0xFFFF)
        {
            continue;
        }
        int idx = Input.BoneIndex[i];
        float weight = Input.BoneWeight[i];
        float4 transformed = mul(ModelPos, SkinningMatrix[idx]);
        skinnedPos += transformed * weight;
    }
    
    Output.Position = skinnedPos;
    
    Output.Position = mul(Output.Position, WorldMatrix);
    
    Output.WorldPosition = Output.Position.xyz;
    
    Output.Position = mul(Output.Position, ViewMatrix);
    Output.Position = mul(Output.Position, ProjectionMatrix);
    
    Output.WorldNormal = mul(Input.Normal, (float3x3)InverseTransposedWorld);

    // Begin Tangent
    float3 WorldTangent = mul(Input.Tangent.xyz, (float3x3)WorldMatrix);
    WorldTangent = normalize(WorldTangent);
    WorldTangent = normalize(WorldTangent - Output.WorldNormal * dot(Output.WorldNormal, WorldTangent));

    Output.WorldTangent = float4(WorldTangent, Input.Tangent.w);
    // End Tangent
    
    Output.UV = Input.UV;
    Output.MaterialIndex = Input.MaterialIndex;

#ifdef LIGHTING_MODEL_GOURAUD
    float3 DiffuseColor = Input.Color;
    if (Material.TextureFlag & TEXTURE_FLAG_DIFFUSE)
    {
        DiffuseColor = DiffuseTexture.SampleLevel(DiffuseSampler, Input.UV, 0).rgb;
    }
    float3 Diffuse = Lighting(Output.WorldPosition, Output.WorldNormal, ViewWorldLocation, DiffuseColor, Material.SpecularColor, Material.Shininess);
    Output.Color = float4(Diffuse.rgb, 1.0);
#else
    Output.Color = float4(1,1,1,1);
#endif
    
    return Output;
}
