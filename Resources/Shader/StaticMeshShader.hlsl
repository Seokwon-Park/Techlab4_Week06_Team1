#pragma pack_matrix(row_major)
cbuffer Viewconstants : register(b0)
{
    matrix VP;
};

cbuffer Worldconstants : register(b2)
{
    matrix World;
};

cbuffer MaterialParams : register(b1)
{
    float4 BaseColor;
    float2 UVOffset;
    float bOpaque;
    float Padding;
};

cbuffer FireBallLightConstants : register(b3)
{
    float4 FireBallPositionRadius;
    float4 FireBallColorIntensity;
    float4 FireBallFalloffEnabled;
};

struct VS_INPUT
{
    float3 p : POSITION; // Input position from vertex buffer
    float3 n : NORMAL;
    float4 c : COLOR; // Input color from vertex buffer
    float2 t : TEXCOORD;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

Texture2D g_txColor : register(t0);
SamplerState g_Sample : register(s0);

// 임시 하드코딩 Directional Light. 빛이 "향하는" 방향이다.
static const float3 LightDir = normalize(float3(0.5f, 0.5f, -1.0f));
static const float3 LightColor = float3(0.5f, 0.5f, 0.5f);
static const float3 AmbientColor = float3(0.5f, 0.5f, 0.5f);

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    output.worldPos = mul(float4(input.p, 1.0f), World).xyz;
    output.position = mul(mul(float4(input.p, 1.0f), World), VP);
    
    // w=0으로 이동 성분을 빼고 월드 공간으로 보낸다. 비균등 스케일이면 역전치가 필요하다.
    output.normal = mul(float4(input.n, 0.0f), World).xyz;
    output.color = input.c;
    output.uv = input.t;
    return output;
}

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float distance = length(FireBallPositionRadius.xyz - input.worldPos);
    float radius = max(FireBallPositionRadius.w, 0.001f);
    
    float falloff = saturate(1.0f - distance / radius);
    falloff = pow(falloff, max(FireBallFalloffEnabled.x, 0.001f));
    
    float enabled = FireBallFalloffEnabled.y;
    float intensity = FireBallColorIntensity.w;
    
    float3 lightDir = normalize(FireBallPositionRadius.xyz - input.worldPos);
    
    // Mesh 표면색
    float4 texColor = g_txColor.Sample(g_Sample, input.uv + UVOffset);
    float4 albedo = texColor * BaseColor;

    float3 N = normalize(input.normal);  // 현재 표면이 향하고 있는 방량
    float pointNdotL = saturate(dot(N, lightDir));
    float directionNdotL = saturate(dot(N, -LightDir));
    float3 lighting = AmbientColor + LightColor * directionNdotL;
    
    // Opaque는 알파를 1로 고정한다. 뷰포트 RT를 ImGui가 알파 블렌딩으로 그리므로 알파가 남으면 비쳐 보인다
    float alpha = bOpaque > 0.5f ? 1.0f : albedo.a;
    
    float3 pointLight = FireBallColorIntensity.rgb * pointNdotL * falloff * enabled * intensity;
    float3 finalRGB = albedo.rgb * (lighting + pointLight);
    return float4(finalRGB, alpha);
}
