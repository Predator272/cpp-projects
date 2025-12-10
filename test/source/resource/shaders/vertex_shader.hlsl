struct vertex_struct
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    uint instance : SV_INSTANCEID;
};

struct pixel_struct
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
};

cbuffer constant_buffer : register(b0)
{
    float4x4 view_projection;
    float4x4 world;
}

StructuredBuffer<float4x4> transformation : register(t0);

pixel_struct main(vertex_struct input)
{
    const pixel_struct output =
    {
        mul(mul(view_projection, mul(world, transformation[input.instance])), float4(input.position, 1.0f)),
        input.texcoord,
        normalize(input.normal),
    };
    return output;
}
