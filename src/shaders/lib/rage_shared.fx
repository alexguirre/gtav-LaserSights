
shared cbuffer rage_clipplanes : register(b0)
{
    float4 ClipPlanes;                          // Offset:    0 Size:    16
}

shared cbuffer rage_matrices : register(b1)
{
    row_major float4x4 gWorld;                  // Offset:    0 Size:    64
    row_major float4x4 gWorldView;              // Offset:   64 Size:    64
    row_major float4x4 gWorldViewProj;          // Offset:  128 Size:    64
    row_major float4x4 gViewInverse;            // Offset:  192 Size:    64
}

shared cbuffer rage_bonemtx : register(b4)
{
    row_major float3x4 gBoneMtx[255];           // Offset:    0 Size: 12240
}

shared cbuffer rage_cbinst_matrices : register(b5)
{
    row_major float4x4 gInstWorldViewProj[6];   // Offset:    0 Size:   384
    row_major float4x4 gInstViewInverse[6];     // Offset:  384 Size:   384
}

shared cbuffer rage_cbinst_update : register(b6)
{
    row_major float4x4 gInstWorld;              // Offset:    0 Size:    64
    uint4 gInstParam[2];                        // Offset:   64 Size:    32
}