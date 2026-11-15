#include "Bindless.h"

#include <array>

#include "Context.h"

namespace pt::vk
{
BindlessDescriptors::BindlessDescriptors(const Context& ctx) : m_ctx(ctx) {
    std::array<VkDescriptorSetLayoutBinding, 3> bindings{};
    bindings[0] = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMaxTextures, VK_SHADER_STAGE_ALL, nullptr };
    bindings[1] = { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kMaxStorageImages, VK_SHADER_STAGE_ALL, nullptr };
    bindings[2] = { 2, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_ALL, nullptr };

    const VkDescriptorBindingFlags bindless =
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
    std::array<VkDescriptorBindingFlags, 3> flags = { bindless, bindless, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT };

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
    flagsInfo.bindingCount = static_cast<u32>(flags.size());
    flagsInfo.pBindingFlags = flags.data();

    VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutInfo.pNext = &flagsInfo;
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    layoutInfo.bindingCount = static_cast<u32>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    VK_CHECK(vkCreateDescriptorSetLayout(ctx.device, &layoutInfo, nullptr, &m_layout));

    std::array<VkDescriptorPoolSize, 3> sizes = { {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, kMaxTextures},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, kMaxStorageImages},
        {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
    } };
    VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = static_cast<u32>(sizes.size());
    poolInfo.pPoolSizes = sizes.data();
    VK_CHECK(vkCreateDescriptorPool(ctx.device, &poolInfo, nullptr, &m_pool));

    VkDescriptorSetAllocateInfo alloc{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    alloc.descriptorPool = m_pool;
    alloc.descriptorSetCount = 1;
    alloc.pSetLayouts = &m_layout;
    VK_CHECK(vkAllocateDescriptorSets(ctx.device, &alloc, &m_set));

    ctx.setName(m_set, VK_OBJECT_TYPE_DESCRIPTOR_SET, "BindlessSet");
}

BindlessDescriptors::~BindlessDescriptors() {
    vkDestroyDescriptorPool(m_ctx.device, m_pool, nullptr);
    vkDestroyDescriptorSetLayout(m_ctx.device, m_layout, nullptr);
}

void BindlessDescriptors::setTexture(u32 slot, VkImageView view, VkSampler sampler) {
    PT_ASSERT(slot < kMaxTextures, "texture slot out of range");
    VkDescriptorImageInfo img{ sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    w.dstSet = m_set;
    w.dstBinding = 0;
    w.dstArrayElement = slot;
    w.descriptorCount = 1;
    w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w.pImageInfo = &img;
    vkUpdateDescriptorSets(m_ctx.device, 1, &w, 0, nullptr);
}

void BindlessDescriptors::setStorageImage(u32 slot, VkImageView view) {
    PT_ASSERT(slot < kMaxStorageImages, "storage image slot out of range");
    VkDescriptorImageInfo img{ VK_NULL_HANDLE, view, VK_IMAGE_LAYOUT_GENERAL };
    VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    w.dstSet = m_set;
    w.dstBinding = 1;
    w.dstArrayElement = slot;
    w.descriptorCount = 1;
    w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    w.pImageInfo = &img;
    vkUpdateDescriptorSets(m_ctx.device, 1, &w, 0, nullptr);
}

void BindlessDescriptors::setTlas(VkAccelerationStructureKHR tlas) {
    VkWriteDescriptorSetAccelerationStructureKHR as{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
    as.accelerationStructureCount = 1;
    as.pAccelerationStructures = &tlas;
    VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    w.pNext = &as;
    w.dstSet = m_set;
    w.dstBinding = 2;
    w.descriptorCount = 1;
    w.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    vkUpdateDescriptorSets(m_ctx.device, 1, &w, 0, nullptr);
}
}