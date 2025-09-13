// Simple Slang shader for rendering generated terrain

[[vk::binding(0, 0)]] cbuffer CameraData {
    float4x4 u_MVP;
};

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
}

struct VSOutput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
}

[shader("vertex")]
VSOutput mainVert(VSInput input) {
    VSOutput output;

    output.position = mul(u_MVP, float4(input.position, 1.0));
    output.normal = input.normal;
    output.texCoord = input.texCoord;

    return output;
}

[shader("fragment")]
float4 mainFrag(VSOutput input) : SV_Target {
    return float4(0.0, 1.0, 0.0, 1.0);
}