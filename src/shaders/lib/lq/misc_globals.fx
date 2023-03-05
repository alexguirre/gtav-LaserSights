
shared cbuffer misc_globals : register(b2)
{
  float4 globalFade;                 // Offset:    0 Size:    16
  float globalHeightScale;           // Offset:   16 Size:     4
  float globalShaderQuality;         // Offset:   20 Size:     4
  float globalReuseMe00001;          // Offset:   24 Size:     4
  float globalReuseMe00002;          // Offset:   28 Size:     4
  float4 g_Rage_Tessellation_CameraPosition;// Offset:   32 Size:    16
  float4 g_Rage_Tessellation_CameraZAxis;// Offset:   48 Size:    16
  float4 g_Rage_Tessellation_ScreenSpaceErrorParams;// Offset:   64 Size:    16
  float4 g_Rage_Tessellation_LinearScale;// Offset:   80 Size:    16
  float4 g_Rage_Tessellation_Frustum[4];// Offset:   96 Size:    64
  float4 g_Rage_Tessellation_Epsilons;// Offset:  160 Size:    16
  float4 globalScalars;              // Offset:  176 Size:    16
  float4 globalScalars2;             // Offset:  192 Size:    16
  float4 globalScalars3;             // Offset:  208 Size:    16
  float4 globalScreenSize;           // Offset:  224 Size:    16 ; xy = screen size, zw = 1 / screen size
  uint4 gTargetAAParams;             // Offset:  240 Size:    16
  float4 colorize;                   // Offset:  256 Size:    16
  float4 gGlobalParticleShadowBias;  // Offset:  272 Size:    16
  float gGlobalParticleDofAlphaScale;// Offset:  288 Size:     4
  float gGlobalFogIntensity;         // Offset:  292 Size:     4
  float4 gPlayerLFootPos;            // Offset:  304 Size:    16
  float4 gPlayerRFootPos;            // Offset:  320 Size:    16
}
