struct VertexOutput
{
    float4 position : SV_Position;
    float3 color : TEXCOORD0;
};

VertexOutput main(uint vertexId : SV_VertexID)
{
    const float2 positions[3] = {
        float2(0.0, -0.7),
        float2(0.7, 0.7),
        float2(-0.7, 0.7)
    };

    const float3 colors[3] = {
        float3(1.0, 0.18, 0.25),
        float3(0.12, 0.86, 0.96),
        float3(1.0, 0.76, 0.14)
    };

    VertexOutput output;
    output.position = float4(positions[vertexId], 0.0, 1.0);
    output.color = colors[vertexId];
    return output;
}
