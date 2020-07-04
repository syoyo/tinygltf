struct IA2VS {
    float3 position  : POSITION;

#ifdef HAS_NORMAL
    float3 normal : NORMAL;
#endif

#ifdef HAS_TANGENT
    float4 tangent : TANGENT;
#endif

#ifdef HAS_TEXCOORD_0
    float2 texcoord_0: TEXCOORD_0;
#endif
};

struct VS2RS {
    float4 position : SV_POSITION;

#ifdef HAS_TEXCOORD_0
    float2 texcoord_0: TEXCOORD_0;
#endif
};

cbuffer Camera : register(b0) {
    float4x4 V;
    float4x4 P;
    float4x4 VP;
};

cbuffer Node : register(b1) {
    float4x4 M;
};

VS2RS main(IA2VS input) {
    VS2RS output;
    output.position = mul(float4(input.position, 1.0), M);
    output.position = mul(output.position, V);
    output.position = mul(output.position, P);

#ifdef HAS_TEXCOORD_0
    output.texcoord_0 = input.texcoord_0;
#endif

    return output;
}