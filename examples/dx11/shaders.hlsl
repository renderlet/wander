
cbuffer constants : register(b0)
{
    row_major float4x4 transform;
    row_major float4x4 projection;
              float3   lightvector;
}

struct vs_in
{
    float3 position : POS;
    float3 normal   : NOR;
    float2 texcoord : TEX;
    float3 color    : COL;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float2 texcoord : TEX;
    float4 color    : COL;
};

Texture2D    mytexture : register(t0);
SamplerState mysampler : register(s0);

vs_out vs_main(vs_in input)
{
    float light = clamp(dot(mul(input.normal, transform), normalize(-lightvector)), 0.0f, 1.0f) * 0.8f + 0.2f;
    
    vs_out output;

    output.position = mul(float4(input.position, 1.0f), mul(transform, projection));
    output.texcoord = input.texcoord;
    output.color    = float4(input.color * light, 1.0f);

    return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    return mytexture.Sample(mysampler, input.texcoord) * input.color;
}
