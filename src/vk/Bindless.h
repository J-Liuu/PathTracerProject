#pragma once

#include "Common.h"

namespace pt::vk
{
class Context;

// one global descriptor set used by every pipeline (set = 0):
//		binding 0 : Sampler2D			textures[kMaxTextures]		(material textures)
//		binding 1 : RWTexture2D<float4>	storageImages[kMaxStorage]	(accumulation, G-buffer, output...)
//		binding 2 : RaytracingAccelerationStructure tlas
// Buffers are NOT bound here. shaders reach them via buffer device address in push constants.
// Slot indices for storage images live in shaders/shared/GpuStructs.h so C++ and Slang agree.
class BindlessDescriptors : NonCopyable
{
public:
	static constexpr u32 kMaxTextures = 4096;
	static constexpr u32 kMaxStorageImages = 32;

	explicit BindlessDescriptors(const Context& ctx);
	~BindlessDescriptors();

	void setTexture(u32 slot, VkImageView view, VkSampler sampler);
	void setStorageImage(u32 slot, VkImageView view);
	void setTlas(VkAccelerationStructureKHR tlas);

	VkDescriptorSetLayout layout() const { return m_layout; }
	VkDescriptorSet set() const { return m_set; }

private:
	const Context& m_ctx;
	VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
	VkDescriptorPool m_pool = VK_NULL_HANDLE;
	VkDescriptorSet m_set = VK_NULL_HANDLE;
};
} // namespace pt::vk