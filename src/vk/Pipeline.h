#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "Common.h"
#include "Resources.h"

namespace pt::vk
{
class Context;

std::vector<u32> loadSpirv(const std::filesystem::path& path);
VkShaderModule createShaderModule(const Context& ctx, const std::vector<u32>& spirv);

// Pipeline layout shared by everything: global bindless set + push constats (see Bindless.h and cpp)
VkPipelineLayout createPipelineLayout(const Context& ctx, VkDescriptorSetLayout globalSet, u32 pushConstantBytes);

// entry point means shader function name under [shader("____")]
VkPipeline createComputePipeline(const Context& ctx, VkPipelineLayout layout, VkShaderModule module, const char* entryPoint, const char* debugName = nullptr);



// ---- RAY TRACING PIPELINE + shader binding table ----------------

struct RayTracingPipelineDesc
{
	VkShaderModule module = VK_NULL_HANDLE;
	const char* rayGen = "rayGen";
	std::vector<const char*> miss = { "miss", "shadowMiss" };
	std::vector<const char*> closestHit = { "closestHit" };
	u32 maxRecursionDepth = 1; // iterative path tracing. we keep this at 1
};

struct RayTracingPipeline
{
	VkPipeline pipeline = VK_NULL_HANDLE;
	Buffer sbt;
	VkStridedDeviceAddressRegionKHR rayGenRegion{};
	VkStridedDeviceAddressRegionKHR missRegion{};
	VkStridedDeviceAddressRegionKHR hitRegion{};
	VkStridedDeviceAddressRegionKHR callableRegion{};
};

RayTracingPipeline createRayTracingPipeline(const Context& ctx, VkPipelineLayout layout, const RayTracingPipelineDesc& desc);

}