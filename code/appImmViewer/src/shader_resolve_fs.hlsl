Texture2DMS<float4> myTex : register(t0);


float3 linear2srgb(float3 val)
{
    if (val.x < 0.0031308) val.x *= 12.92; else val.x = 1.055*pow(abs(val.x), 1.0 / 2.4) - 0.055;
    if (val.y < 0.0031308) val.y *= 12.92; else val.y = 1.055*pow(abs(val.y), 1.0 / 2.4) - 0.055;
    if (val.z < 0.0031308) val.z *= 12.92; else val.z = 1.055*pow(abs(val.z), 1.0 / 2.4) - 0.055;
    return val;
}

float4 main( float4 fragCoord : SV_POSITION ) : SV_TARGET
{
    float3 col = float3(0.0,0.0,0.0);
    for( int i=0; i<8; i++ )
        col += myTex.Load(int2(fragCoord.xy), i).xyz;
    col /= 8.0;

    col = linear2srgb(col);

    return float4(col,1.0);
}