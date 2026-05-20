// CPUから受け取る頂点データの形
// Mesh.hで定義した「struct Vertex」と完全に一致させる必要がある。
// --- ここが C++ の "POSITION" や "COLOR" と完全に一致している必要があります ---
struct VS_INPUT
{
    float3 Pos : POSITION;
    float4 Color : COLOR;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.Pos = float4(input.Pos, 1.0f);
    output.Color = input.Color;
    return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    return input.Color;
}