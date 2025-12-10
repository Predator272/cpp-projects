struct VERTEX_SHADER_INPUT
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
};

struct PIXEL_SHADER_INPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
};

cbuffer CONSTANT_BUFFER : register(b0)
{
    float4x4 view_projection;
    float4x4 world;
}

PIXEL_SHADER_INPUT main(VERTEX_SHADER_INPUT input)
{
    const PIXEL_SHADER_INPUT output =
    {
        mul(mul(view_projection, world), float4(input.position, 1.0f)),
        input.texcoord,
        normalize(input.normal),
    };
    return output;
}
