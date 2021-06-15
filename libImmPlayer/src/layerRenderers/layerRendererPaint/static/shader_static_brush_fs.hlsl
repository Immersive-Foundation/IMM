#define CUSTOM_ALPHA_TO_COVERAGE 1
#define MSAASampleCount 8

cbuffer FrameState : register(b0)
{
    struct
    {
        float       mTime;
        int         mFrame;
        int         dummy1;
        int         dummy2;
    }frame;
};

float4 main(
            #if STEREOMODE==2	
            float clip : SV_ClipDistance0,
            #endif
            float4 color : V2P_COLOR,
            float4 fragCoord : SV_POSITION, 
            #if CUSTOM_ALPHA_TO_COVERAGE==1
            uint primitiveID : V2P_INFO,
            out uint coverage : SV_COVERAGE
            #else
            uint primitiveID : V2P_INFO
            #endif
           ) : SV_TARGET
{

    float2 q = fragCoord.xy + float2(0,float((frame.mFrame) & 7)*11.0);

    float ran = frac( 52.9829189*frac(dot(q,float2(0.06711056,0.00583715))));

    float al = clamp(color.a + 0.99*(ran - 0.5) / float(MSAASampleCount), 0.0, 1.0); // 0.99 is to make sure the dithering never makes the alpha leak to the previour or the next bucket

    #if CUSTOM_ALPHA_TO_COVERAGE==1
    color.a = 1.0;
    uint mask = (0xff00 >> uint(al*float(MSAASampleCount) + 0.5)) & 0xff;  // compute sample mask from input alpha, like hardware is doing it
    uint shift = (uint(ran*7.0) + primitiveID) & 7;                        // randomize mask per primitive ID and pixel
    coverage = (((mask << 8) | mask) >> shift) & 0xff;                     // barrel Shift 8 Bit
    #else
    color.a = al;
    #endif
/*
#if STEREOMODE==0	
    color.xyz *= float3(1.0, 0.0, 0.0);
#endif
#if STEREOMODE==1
    color.xyz *= float3(0.0, 1.0, 0.0);
#endif
#if STEREOMODE==2	
    color.xyz *= float3(0.0, 0.0, 1.0);
#endif
*/
    return color;
}