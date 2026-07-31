struct RS2PS {
    float4 position : SV_POSITION;

#ifdef HAS_TEXCOORD_0
    float2 texcoord_0: TEXCOORD_0;
#endif
};

struct TextureInfo {
    uint textureIndex;
    uint samplerIndex;
};

struct PBRMetallicRoughness {
    float4 baseColorFactor;
    TextureInfo baseColorTexture;
    float metallicFactor;
    float roughnessFactor;
    TextureInfo metallicRoughnessTexture;
};

cbuffer Material : register(b2) {
    PBRMetallicRoughness pbrMetallicRoughness;
};

Texture2D textures[5] : register(t0);
SamplerState samplerState[5] : register(s0);

float4 getBaseColor(float2 uv) {
    float4 baseColor = pbrMetallicRoughness.baseColorFactor;

#ifdef HAS_TEXCOORD_0
    TextureInfo baseColorTexture = pbrMetallicRoughness.baseColorTexture;
    if (baseColorTexture.textureIndex >= 0) {
        baseColor *= textures[baseColorTexture.textureIndex].Sample(samplerState[baseColorTexture.samplerIndex], uv);
    }
#endif

    return baseColor;
}

float4 main(RS2PS input) : SV_Target {
    float2 uv = float2(0.0, 0.0);

#ifdef HAS_TEXCOORD_0
    uv = input.texcoord_0;
#endif

    float4 color = getBaseColor(uv);

    return color;
}
