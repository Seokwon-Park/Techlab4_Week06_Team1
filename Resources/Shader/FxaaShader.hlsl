Texture2D InputTexture : register(t0);
SamplerState LinearClamp : register(s0);

//절대 임계값
#define EDGE_THRESHOLD_MIN 0.0312f
//상대 임계값
#define EDGE_THRESHOLD_MAX 0.125f

#define QUALITY(q) ((q) < 5 ? 1.0 : ((q) > 5 ? ((q) < 10 ? 2.0 : ((q) < 11 ? 4.0 : 8.0)) : 1.5))

static const int ITERATIONS = 12;

static const float SUBPIXEL_QUALITY = 0.75f;

cbuffer FXAAConstants : register(b0)
{
    float2 InverseScreenSize;
    float2 Padding;
};

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

PSInput mainVS(uint VertexID : SV_VertexID)
{
    PSInput Output;
    float2 UV = float2((VertexID << 1) & 2, VertexID & 2); // (0,0) (2,0) (0,2)
    Output.Position = float4(UV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    Output.UV = UV;
    return Output;
}

//밝기 구하는 함수
float rgb2luma(float3 rgb)
{
    return dot(rgb, float3(0.299f, 0.587f, 0.114f));
}

//루프 안에서도 쓰므로 미분값이 필요 없는 SampleLevel로 읽는다 
float3 SampleColor(float2 uv)
{
    return InputTexture.SampleLevel(LinearClamp, uv, 0).rgb;
}

//현재 UV에서 픽셀 (x, y)칸 떨어진 곳의 밝기. D3D는 v가 아래로 증가하므로 y = -1이 위쪽이다
float SampleLumaOffset(float2 uv, float x, float y)
{
    return rgb2luma(SampleColor(uv + float2(x, y) * InverseScreenSize));
}

float4 mainPS(PSInput input) : SV_Target
{
    float3 colorCenter = SampleColor(input.UV);

    //해당 픽셀의 밝기
    float lumaCenter = rgb2luma(colorCenter);

    //1.AA를 적용할 픽셀인지 판단
    //-----------------------------------------------------------------------------------
    float lumaDown = SampleLumaOffset(input.UV, 0.0f, 1.0f);
    float lumaUp = SampleLumaOffset(input.UV, 0.0f, -1.0f);
    float lumaLeft = SampleLumaOffset(input.UV, -1.0f, 0.0f);
    float lumaRight = SampleLumaOffset(input.UV, 1.0f, 0.0f);

    //최소,최대 밝기
    float lumaMin = min(lumaCenter, min(min(lumaDown, lumaUp), min(lumaLeft, lumaRight)));
    float lumaMax = max(lumaCenter, max(max(lumaDown, lumaUp), max(lumaLeft, lumaRight)));
    
    float lumaRange = lumaMax - lumaMin;
    
    //밝기의 변화가 임계값보다 작으면 AA를 적용하지 않음
    //0에 가까운 매우 어두운 픽셀은 절대 임계값을 사용하게하고, 밝은 픽셀들은 상대 임계값을 사용하게 함(밝을 수록 높게 임계값을 잡은것)
    if (lumaRange < max(EDGE_THRESHOLD_MIN, lumaMax * EDGE_THRESHOLD_MAX))
    {
        return float4(colorCenter, 1.0f);
    };
    //-----------------------------------------------------------------------------------   
    
    
    //2. AA를 적용할 픽셀이라면, 주변 픽셀의 색을 샘플링하여 경계선의 방향을 계산
    //-----------------------------------------------------------------------------------
    //대각선 방향 계산
    float lumaDownLeft = SampleLumaOffset(input.UV, -1.0f, 1.0f);
    float lumaUpRight = SampleLumaOffset(input.UV, 1.0f, -1.0f);
    float lumaUpLeft = SampleLumaOffset(input.UV, -1.0f, -1.0f);
    float lumaDownRight = SampleLumaOffset(input.UV, 1.0f, 1.0f);


    float lumaDownUp = lumaDown + lumaUp;
    float lumaLeftRight = lumaLeft + lumaRight;


    float lumaLeftCorners = lumaDownLeft + lumaUpLeft;
    float lumaDownCorners = lumaDownLeft + lumaDownRight;
    float lumaRightCorners = lumaDownRight + lumaUpRight;
    float lumaUpCorners = lumaUpRight + lumaUpLeft;


    float edgeHorizontal = abs(-2.0 * lumaLeft + lumaLeftCorners) + abs(-2.0 * lumaCenter + lumaDownUp) * 2.0 + abs(-2.0 * lumaRight + lumaRightCorners);
    float edgeVertical = abs(-2.0 * lumaUp + lumaUpCorners) + abs(-2.0 * lumaCenter + lumaLeftRight) * 2.0 + abs(-2.0 * lumaDown + lumaDownCorners);

    bool isHorizontal = (edgeHorizontal >= edgeVertical);
    //-----------------------------------------------------------------------------------
    
    
    //3. 경계선의 방향에 따라, 해당 방향으로의 밝기 변화량을 계산해서 오른쪽인지 왼쪽인지 판단
    //-----------------------------------------------------------------------------------
    
    //수평방향이면 위, 아래 픽셀의 밝기 변화량을 계산하고, 수직방향이면 좌, 우 픽셀의 밝기 변화량을 계산
    float luma1 = isHorizontal ? lumaUp : lumaLeft;
    float luma2 = isHorizontal ? lumaDown : lumaRight;
    
    //밝기 변화량 계산
    float gradient1 = luma1 - lumaCenter;
    float gradient2 = luma2 - lumaCenter;

    //밝기 변화량이 큰 쪽이 경계선의 방향
    bool is1Steepest = abs(gradient1) >= abs(gradient2);

    //밝기 변화량이 큰 쪽의 절대값을 0.25배로 줄여서, 경계선의 방향으로 이동할 픽셀의 밝기 변화량을 계산
    float gradientScaled = 0.25 * max(abs(gradient1), abs(gradient2));

    //경계선의 방향으로 이동할 픽셀의 밝기 변화량을 계산
    float stepLength = isHorizontal ? InverseScreenSize.y : InverseScreenSize.x;

    float lumaLocalAverage = 0.0;

    if (is1Steepest)
    {
        //방향 전환
        stepLength = -stepLength;
        lumaLocalAverage = 0.5 * (luma1 + lumaCenter);
    }
    else
    {
        lumaLocalAverage = 0.5 * (luma2 + lumaCenter);
    }

    //픽셀의 중간 위치를 계산
    float2 currentUV = input.UV;
    if (isHorizontal)
    {
        currentUV.y += stepLength * 0.5;
    }
    else
    {
        currentUV.x += stepLength * 0.5;
    }
    //--------------------------------------------------------------------------------
    
    //4. 경계선의 방향으로 이동하면서, 밝기 변화량이 gradientScaled보다 작아지는 지점을 찾음
    //--------------------------------------------------------------------------------

    float2 offset = isHorizontal ? float2(InverseScreenSize.x, 0.0) : float2(0.0, InverseScreenSize.y);

    float2 uv1 = currentUV - offset;
    float2 uv2 = currentUV + offset;


    float lumaEnd1 = rgb2luma(SampleColor(uv1));
    float lumaEnd2 = rgb2luma(SampleColor(uv2));
    lumaEnd1 -= lumaLocalAverage;
    lumaEnd2 -= lumaLocalAverage;

    bool reached1 = abs(lumaEnd1) >= gradientScaled;
    bool reached2 = abs(lumaEnd2) >= gradientScaled;
    bool reachedBoth = reached1 && reached2;


    if (!reached1)
    {
        uv1 -= offset;
    }
    if (!reached2)
    {
        uv2 += offset;
    }
    if (!reachedBoth)
    {

        for (int i = 2; i < ITERATIONS; i++)
        {

            if (!reached1)
            {
                lumaEnd1 = rgb2luma(SampleColor(uv1));
                lumaEnd1 = lumaEnd1 - lumaLocalAverage;
            }

            if (!reached2)
            {
                lumaEnd2 = rgb2luma(SampleColor(uv2));
                lumaEnd2 = lumaEnd2 - lumaLocalAverage;
            }

            reached1 = abs(lumaEnd1) >= gradientScaled;
            reached2 = abs(lumaEnd2) >= gradientScaled;
            reachedBoth = reached1 && reached2;


            if (!reached1)
            {
                uv1 -= offset * QUALITY(i);
            }
            if (!reached2)
            {
                uv2 += offset * QUALITY(i);
            }


            if (reachedBoth)
            {
                break;
            }
        }
    }
    //--------------------------------------------------------------------------------
    
    //5. Offset 계산
    //--------------------------------------------------------------------------------

    float distance1 = isHorizontal ? (input.UV.x - uv1.x) : (input.UV.y - uv1.y);
    float distance2 = isHorizontal ? (uv2.x - input.UV.x) : (uv2.y - input.UV.y);


    bool isDirection1 = distance1 < distance2;
    float distanceFinal = min(distance1, distance2);


    float edgeThickness = (distance1 + distance2);


    float pixelOffset = -distanceFinal / edgeThickness + 0.5;
    

    bool isLumaCenterSmaller = lumaCenter < lumaLocalAverage;

    bool correctVariation = ((isDirection1 ? lumaEnd1 : lumaEnd2) < 0.0) != isLumaCenterSmaller;


    float finalOffset = correctVariation ? pixelOffset : 0.0;
    //--------------------------------------------------------------------------------
    
    //6.subpixel AA적용
    //--------------------------------------------------------------------------------
    // Sub-pixel shifting
    float lumaAverage = (1.0 / 12.0) * (2.0 * (lumaDownUp + lumaLeftRight) + lumaLeftCorners + lumaRightCorners);

    float subPixelOffset1 = clamp(abs(lumaAverage - lumaCenter) / lumaRange, 0.0, 1.0);
    float subPixelOffset2 = (-2.0 * subPixelOffset1 + 3.0) * subPixelOffset1 * subPixelOffset1;

    float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;


    finalOffset = max(finalOffset, subPixelOffsetFinal);
    //--------------------------------------------------------------------------------
    
    //7. 최종 픽셀 색상 계산
    float2 finalUv = input.UV;
    if (isHorizontal)
    {
        finalUv.y += finalOffset * stepLength;
    }
    else
    {
        finalUv.x += finalOffset * stepLength;
    }

    float3 finalColor = SampleColor(finalUv);
    return float4(finalColor, 1.0f);
}