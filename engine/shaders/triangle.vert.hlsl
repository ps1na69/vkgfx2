struct VertexInput
{
    float2 position : TEXCOORD0;
    float3 color : TEXCOORD1;
};

cbuffer TransformData : register(b0, space1)
{
    row_major float4x4 model;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color : TEXCOORD0;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.position = mul(model, float4(input.position, 0.0, 1.0));
    output.color = input.color;
    return output;
}
