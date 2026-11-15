#pragma once

#include <utility>
#include <vector>

#include "Common.h"
#include "Resources.h"

// #######################
// AI generated header & cpp
// #########################

namespace pt::vk {

    class Context;

    struct AccelStruct {
        VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
        Buffer buffer;
        VkDeviceAddress address = 0;

        AccelStruct() = default;
        AccelStruct(AccelStruct&& o) noexcept { *this = std::move(o); }
        AccelStruct& operator=(AccelStruct&& o) noexcept {
            handle = std::exchange(o.handle, VK_NULL_HANDLE);
            buffer = std::move(o.buffer);
            address = std::exchange(o.address, 0);
            return *this;
        }

        void destroy(VkDevice device);
    };

    struct BlasInput {
        VkDeviceAddress vertexAddress = 0;   // float3 position at offset 0
        u32 vertexStride = 0;
        u32 vertexCount = 0;
        VkDeviceAddress indexAddress = 0;    // u32 indices
        u32 triangleCount = 0;
        bool opaque = true;                  // opaque => no any-hit invocations (fast)
    };

    struct TlasInstance {
        mat4 transform{ 1.0f };
        u32 customIndex = 0;                 // InstanceID() in shaders -> GpuMeshInfo index
        VkDeviceAddress blasAddress = 0;
        u8 mask = 0xFF;
    };

    AccelStruct buildBlas(const Context& ctx, VkCommandBuffer cmd, const BlasInput& in, std::vector<Buffer>& scratchOut);

    AccelStruct buildTlas(const Context& ctx, VkCommandBuffer cmd, const std::vector<TlasInstance>& instances,
        std::vector<Buffer>& scratchOut);

} // namespace pt::vk