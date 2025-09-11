// Simple Slang shader for rendering generated terrain

struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
}

struct VSOutput {
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float2 texCoord;
}

[shader("vertex")]
VSOutput mainVert(VSInput input) {
    VSOutput output;

    output.position = float4(input.position, 1.0);
    output.normal = input.normal;
    output.texCoord = input.texCoord;

    return output;
}

[shader("fragment")]
float4 mainFrag(VSOutput input) : SV_Target {
    return float4(1.0, 0.0, 0.0, 1.0);
}