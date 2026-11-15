#include "AccelStruct.h"

#include "Context.h"
// #######################
// AI generated header & cpp
// All accel structure related code is generated
// #########################



namespace pt::vk {

namespace {
VkDeviceSize alignUp(VkDeviceSize v, VkDeviceSize a) { return (v + a - 1) & ~(a - 1); }


AccelStruct createAccelStruct(const Context& ctx, VkAccelerationStructureTypeKHR type, VkDeviceSize size,
    const char* name)
{
    AccelStruct as;
    as.buffer = Buffer(ctx, size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
        MemoryUsage::GpuOnly, name);

    VkAccelerationStructureCreateInfoKHR info{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    info.buffer = as.buffer.handle();
    info.offset = 0; // must be a multiple of 256
    info.size = size;
    info.type = type;
    VK_CHECK(vkCreateAccelerationStructureKHR(ctx.device, &info, nullptr, &as.handle));

    VkAccelerationStructureDeviceAddressInfoKHR addrInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
    addrInfo.accelerationStructure = as.handle;
    as.address = vkGetAccelerationStructureDeviceAddressKHR(ctx.device, &addrInfo);

    if (name) ctx.setName(as.handle, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, name);
    return as;
}

// Allocates a scratch buffer and returns a device address aligned to
// minAccelerationStructureScratchOffsetAlignment (VMA doesn't guarantee it, so over-allocate and round up).
// The buffer is pushed into `keepAlive`: it must stay alive until the GPU has executed the build.
VkDeviceAddress allocateScratch(const Context& ctx, VkDeviceSize size, std::vector<Buffer>& keepAlive)
{
    const VkDeviceSize align = ctx.caps.accelStruct.minAccelerationStructureScratchOffsetAlignment;
    keepAlive.emplace_back(ctx, size + align, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryUsage::GpuOnly,
        "AS scratch");
    return alignUp(keepAlive.back().address(), align);
}

}



void AccelStruct::destroy(VkDevice device) {
    if (handle) vkDestroyAccelerationStructureKHR(device, handle, nullptr);
    handle = VK_NULL_HANDLE;
    buffer.destroy();
    address = 0;
}

AccelStruct buildBlas(const Context& ctx, VkCommandBuffer cmd, const BlasInput& in, std::vector<Buffer>& scratchOut) {
    PT_ASSERT(in.triangleCount > 0 && in.vertexCount > 0, "buildBlas: empty mesh");

    // 1. Where the triangles are (all GPU addresses, nothing is copied)
    VkAccelerationStructureGeometryTrianglesDataKHR tri{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
    tri.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;    // GpuVertex::position (offset 0)
    tri.vertexData.deviceAddress = in.vertexAddress;
    tri.vertexStride = in.vertexStride;               // 32 = sizeof(GpuVertex); normal/uv are skipped
    tri.maxVertex = in.vertexCount - 1;               // highest vertex index any triangle uses
    tri.indexType = VK_INDEX_TYPE_UINT32;
    tri.indexData.deviceAddress = in.indexAddress;

    VkAccelerationStructureGeometryKHR geom{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    geom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geom.geometry.triangles = tri;
    geom.flags = in.opaque ? VkGeometryFlagsKHR(VK_GEOMETRY_OPAQUE_BIT_KHR) : 0u;   // opaque: no any-hit calls

    // 2. What kind of build
    VkAccelerationStructureBuildGeometryInfoKHR build{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
    build.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    build.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;   // static mesh: optimise for tracing
    build.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build.geometryCount = 1;
    build.pGeometries = &geom;

    // 3. Ask the driver how much memory the BLAS and the build workspace need
    VkAccelerationStructureBuildSizesInfoKHR sizes{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    vkGetAccelerationStructureBuildSizesKHR(ctx.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build,
        &in.triangleCount, &sizes);

    // 4. Allocate the BLAS + scratch, then record the build
    AccelStruct blas = createAccelStruct(ctx, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        sizes.accelerationStructureSize, "BLAS");
    build.dstAccelerationStructure = blas.handle;
    build.scratchData.deviceAddress = allocateScratch(ctx, sizes.buildScratchSize, scratchOut);

    VkAccelerationStructureBuildRangeInfoKHR range{};
    range.primitiveCount = in.triangleCount;          // primitiveOffset/firstVertex stay 0: addresses already point at the mesh
    const VkAccelerationStructureBuildRangeInfoKHR* pRange = &range;
    vkCmdBuildAccelerationStructuresKHR(cmd, 1, &build, &pRange);

    return blas;   // address is valid now; contents are valid once the command buffer has executed
}

AccelStruct buildTlas(const Context& ctx, VkCommandBuffer cmd, const std::vector<TlasInstance>& instances, std::vector<Buffer>& keepAlive)
{
    PT_ASSERT(!instances.empty(), "buildTlas: no instances");

    // 1. One instance record per scene instance (CPU side)
    std::vector<VkAccelerationStructureInstanceKHR> records(instances.size());
    for (size_t i = 0; i < instances.size(); ++i) {
        const TlasInstance& src = instances[i];
        VkAccelerationStructureInstanceKHR& dst = records[i];

        // Vulkan wants a 3x4 ROW-major matrix; glm::mat4 is COLUMN-major (m[column][row]).
        // So transpose while copying. Getting this wrong = mirrored/sheared/missing objects.
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 4; ++col)
                dst.transform.matrix[row][col] = src.transform[col][row];

        dst.instanceCustomIndex = src.customIndex & 0xFFFFFFu;    // 24 bits -> InstanceID() in shaders
        dst.mask = src.mask;                                       // 8 bits, matched against TraceRay's mask
        dst.instanceShaderBindingTableRecordOffset = 0;            // one hit group for everything
        dst.flags = static_cast<u32>(VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR); // hit both sides
        dst.accelerationStructureReference = src.blasAddress;      // the BLAS *device address*
    }

    // 2. Upload them. The build reads this buffer on the GPU, so it must stay alive until the
    //    build has executed -> it goes into keepAlive together with the scratch buffer.
    const VkDeviceSize bytes = sizeof(VkAccelerationStructureInstanceKHR) * records.size();
    keepAlive.emplace_back(ctx, bytes, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        MemoryUsage::Upload, "TLAS instances");
    Buffer& instanceBuffer = keepAlive.back();
    instanceBuffer.upload(records.data(), bytes);   // host writes before vkQueueSubmit are visible to the GPU
    PT_ASSERT(instanceBuffer.address() % 16 == 0, "TLAS instance data must be 16-byte aligned");

    // 3. Geometry = "an array of instances at this address"
    VkAccelerationStructureGeometryInstancesDataKHR instData{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
    instData.arrayOfPointers = VK_FALSE;
    instData.data.deviceAddress = instanceBuffer.address();

    VkAccelerationStructureGeometryKHR geom{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    geom.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geom.geometry.instances = instData;

    VkAccelerationStructureBuildGeometryInfoKHR build{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
    build.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    build.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;   // later: | ALLOW_UPDATE for moving objects
    build.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build.geometryCount = 1;
    build.pGeometries = &geom;

    // 4. Sizes -> allocate -> record the build (same pattern as the BLAS)
    const u32 instanceCount = static_cast<u32>(records.size());
    VkAccelerationStructureBuildSizesInfoKHR sizes{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    vkGetAccelerationStructureBuildSizesKHR(ctx.device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build,
        &instanceCount, &sizes);

    AccelStruct tlas = createAccelStruct(ctx, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        sizes.accelerationStructureSize, "TLAS");
    build.dstAccelerationStructure = tlas.handle;
    build.scratchData.deviceAddress = allocateScratch(ctx, sizes.buildScratchSize, keepAlive);

    VkAccelerationStructureBuildRangeInfoKHR range{};
    range.primitiveCount = instanceCount;
    const VkAccelerationStructureBuildRangeInfoKHR* pRange = &range;
    vkCmdBuildAccelerationStructuresKHR(cmd, 1, &build, &pRange);
    return tlas;
}
}