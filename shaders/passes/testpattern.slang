// test compute
// fills accumulation image w HDR gradient so tonemap pass has input

#include "common/bindings.slangh"

// [shader("compute")]
// [[numthreads(8, 8, 1)]]
// void testPattern(uint3 tid: SV_DispatchThreadID)
// {
//     uint2 pixel = tid.xy;
//     if (any(pixel >= pc.extent)) return;
//     FrameUniforms frame = pc.frame[0];

//     float2 uv = (float2(pixel) + 0.5) / float2(pc.extent);
//     float t = float(frame.frameIndex) * 0.01;

//     // RayDesc r = primaryRay(frame, pixel, pc.extent, float2(0.4, 0.4));
//     // gImages[kSlotAccumulation][pixel] = float4(abs(r.Direction), 1.0);
    
//     // deliberately HDR so we can test ACES compressing highlights
//     float3 hdr = float3(uv.x, uv.y, 0.5 + 0.5 * sin(t)) * 4.0 * uv.x;
//     gImages[kSlotAccumulation][pixel] = float4(hdr, 1.0);

//     // const uint kMaterialCount = 5;
//     // uint m = min(pixel.x * kMaterialCount / pc.extent.x, kMaterialCount - 1);
//     // GpuMaterial mat = pc.materials[m];
//     // float3 c = mat.baseColor + mat.emission * 0.05; 
//     // // also check a mesh info from instance 0:
//     // if (pixel.y < 20) c = float3(pc.meshes[0].triangleCount == 2 ? 0 : 1, pc.meshes[0].triangleCount == 2 ? 1 : 0, 0);
//     // gImages[kSlotAccumulation][pixel] = float4(c, 1.0);
// }

[shader("compute")]
[numthreads(8, 8, 1)]
void testPattern(uint3 tid: SV_DispatchThreadID)
{
    uint2 pixel = tid.xy;
    if (any(pixel >= pc.extent)) return;
    FrameUniforms frame = pc.frame[0];

    RayDesc ray = primaryRay(frame, pixel, pc.extent, float2(0.5, 0.5));
    RayQuery<RAY_FLAG_NONE> q;
    q.TraceRayInline(gTlas, RAY_FLAG_NONE, 0xFF, ray);
    while (q.Proceed()) {} // opaque geometry: one call finishes, the loop is just correct form

    float3 color = float3(0.0); // miss = black
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        uint id = q.CommittedInstanceID(); // = customIndex = instance index
        float3 idColor = frac(sin(float3(id + 1) * float3(12.9898, 78.233, 45.164)) * 43758.5453);
        float fade = 1.0 / (1.0 + 0.15 * q.CommittedRayT()); // nearer = brighter
        color = idColor * fade;
    }
    gImages[kSlotAccumulation][pixel] = float4(color, 1.0);
}