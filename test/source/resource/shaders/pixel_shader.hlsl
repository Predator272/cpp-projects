struct pixel_struct
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
};

Texture2D diffuse_texture : register(t0);
SamplerState default_sampler : register(s0);

float4 main(pixel_struct input) : SV_TARGET
{
    return diffuse_texture.Sample(default_sampler, input.texcoord);
}
