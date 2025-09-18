// Simple Slang shader for rendering generated terrain

[[vk::binding(0, 0)]] cbuffer CameraData {
    float4x4 u_MVP;
    float4x4 u_normalMatrix;
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
    output.normal = float3(mul(u_normalMatrix, float4(input.normal, 1.0)).xyz);
    output.texCoord = input.texCoord;

    return output;
}

[shader("fragment")]
float4 mainFrag(VSOutput input) : SV_Target {
    float ambient = 0.1;
    float3 direction = normalize(float3(0.5, 1.0, 0.5));
    float intensity = 0.9;

    float3 lambert = max(0.0, dot(input.normal, direction)) * intensity + float3(ambient);

    return float4(lambert, 1.0);
}