
// c++からマイフレーム送られてくる定数バッファ（箱）
// register(b0)は、「0番目の定数バッファスロットを使う」という指定
cbuffer TransformBuffer : register(b0) // 描画したオブジェクトをImGuiで動かしたりサイズを変える時に書いたプログラム
{
    matrix World; // ワールド座標
    matrix View;
    matrix projection;
    
    float4 materialColor; // C++から送られてきた色を受け取る
    
    int useSolidColor;
    float3 dummy;
}

// C++から送られてくる「画像」と「サンプラー」
Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

// CPUから受け取る頂点データの形
// Mesh.hで定義した「struct Vertex」と完全に一致させる必要がある。
// --- ここが C++ の "POSITION" や "COLOR" と完全に一致している必要があります ---
struct VS_INPUT
{
    float3 Pos : POSITION;
    float4 Color : COLOR;
    float2 Tex : TEXCOORD; // UV座標
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
    float2 Tex : TEXCOORD; // UV座標
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    
    float4 localPos = float4(input.Pos, 1.0f);
    
    output.Pos = mul(localPos, World); // 頂点のローカル座標に、ワールド座標を掛け算(mul)して移動させる。
    output.Pos = mul(output.Pos, View); // カメラからの視点に変換
    output.Pos = mul(output.Pos, projection); // 遠近感
    output.Color = input.Color;
    output.Tex = input.Tex; // そのままピクセルシェーダーへ送る
    return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    // スイッチがONなら単色（頂点カラー無視）
    if (useSolidColor ==1)
    {
        return materialColor;
    }
    // スイッチがOFFなら虹色にする
    else if(useSolidColor == 2)
    {
       // 画像を持たないグリッド線
        return input.Color * materialColor;
    }
    else
    {
         // UV座標を使って、画像（テクスチャ）から色を読み取る
        float4 texColor = txDiffuse.Sample(samLinear, input.Tex);
        
        return texColor * materialColor;
    }
    
}