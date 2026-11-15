#include "Pipeline.h"

#include <fstream>

#include "Context.h"

namespace pt::vk
{

namespace
{
VkDeviceSize alignUp(VkDeviceSize v, VkDeviceSize a) { return (v + a - 1) & ~(a - 1); }
}


// ai generated ---
std::vector<u32> loadSpirv(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        log::error("Cannot open shader {}", path.string());
        return {};
    }
    const auto size = static_cast<size_t>(file.tellg());
    PT_ASSERT(size % 4 == 0, "SPIR-V size must be a multiple of 4");
    std::vector<u32> words(size / 4);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(words.data()), static_cast<std::streamsize>(size));
    return words;
}
//-----------------

VkShaderModule createShaderModule(const Context& ctx, const std::vector<u32>& spirv)
{
    VkShaderModuleCreateInfo info{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    info.codeSize = spirv.size() * sizeof(u32);
    info.pCode = spirv.data();
    VkShaderModule module = VK_NULL_HANDLE;
    VK_CHECK(vkCreateShaderModule(ctx.device, &info, nullptr, &module));
    return module;
}

VkPipelineLayout createPipelineLayout(const Context& ctx, VkDescriptorSetLayout globalSet, u32 pushConstantBytes) {
    VkPushConstantRange range{};
    range.stageFlags = VK_SHADER_STAGE_ALL;
    range.size = pushConstantBytes;

    VkPipelineLayoutCreateInfo info{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    info.setLayoutCount = globalSet ? 1u : 0u;
    info.pSetLayouts = &globalSet;
    info.pushConstantRangeCount = pushConstantBytes ? 1u : 0u;
    info.pPushConstantRanges = &range;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VK_CHECK(vkCreatePipelineLayout(ctx.device, &info, nullptr, &layout));
    return layout;
}

VkPipeline createComputePipeline(const Context& ctx, VkPipelineLayout layout, VkShaderModule module,
    const char* entryPoint, const char* debugName) {
    VkComputePipelineCreateInfo info{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    info.stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    info.stage.module = module;
    info.stage.pName = entryPoint;
    info.layout = layout;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VK_CHECK(vkCreateComputePipelines(ctx.device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline));
    if (debugName) ctx.setName(pipeline, VK_OBJECT_TYPE_PIPELINE, debugName);
    return pipeline;
}

RayTracingPipeline createRayTracingPipeline(const Context& ctx, VkPipelineLayout layout,
    const RayTracingPipelineDesc& desc)
{
    
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;

    auto addStage = [&](VkShaderStageFlagBits stage, const char* entry) {
        VkPipelineShaderStageCreateInfo s{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        s.stage = stage;
        s.module = desc.module; // one .spv holds all entry points (-fvk-use-entrypoint-name)
        s.pName = entry;
        stages.push_back(s);
        return static_cast<u32>(stages.size() - 1);
        };
    auto addGroup = [&](VkRayTracingShaderGroupTypeKHR type, u32 general, u32 closestHit) {
        VkRayTracingShaderGroupCreateInfoKHR g{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
        g.type = type;
        g.generalShader = general; // raygen / miss / callable
        g.closestHitShader = closestHit; // hit groups
        g.anyHitShader = VK_SHADER_UNUSED_KHR;
        g.intersectionShader = VK_SHADER_UNUSED_KHR;
        groups.push_back(g);
        };

    // ai generated for sbt
    // group order = SBT order: [raygen] [miss...] [hit...]
    addGroup(VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        addStage(VK_SHADER_STAGE_RAYGEN_BIT_KHR, desc.rayGen), VK_SHADER_UNUSED_KHR);
    for (const char* m : desc.miss)
        addGroup(VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
            addStage(VK_SHADER_STAGE_MISS_BIT_KHR, m), VK_SHADER_UNUSED_KHR);
    for (const char* h : desc.closestHit)
        addGroup(VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
            VK_SHADER_UNUSED_KHR, addStage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, h));

    // pipeline ------------------------------------------------------------
    VkRayTracingPipelineCreateInfoKHR info{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
    info.stageCount = static_cast<u32>(stages.size());
    info.pStages = stages.data();
    info.groupCount = static_cast<u32>(groups.size());
    info.pGroups = groups.data();
    info.maxPipelineRayRecursionDepth = desc.maxRecursionDepth;   // we loop in raygen. no recursion currently.
    info.layout = layout;

    RayTracingPipeline out;
    VK_CHECK(vkCreateRayTracingPipelinesKHR(ctx.device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &info, nullptr,
        &out.pipeline));


    // ai
    // shader binding table ------------------------------------------------
    const auto& rt = ctx.caps.rtPipeline;
    const VkDeviceSize handleSize = rt.shaderGroupHandleSize;                           // usually 32
    const VkDeviceSize handleStride = alignUp(handleSize, rt.shaderGroupHandleAlignment); // record size
    const VkDeviceSize baseAlign = rt.shaderGroupBaseAlignment;                         // usually 64
    const u32 missCount = static_cast<u32>(desc.miss.size());
    const u32 hitCount = static_cast<u32>(desc.closestHit.size());

    out.rayGenRegion.stride = alignUp(handleStride, baseAlign);
    out.rayGenRegion.size = out.rayGenRegion.stride;   // spec: raygen size must equal stride
    out.missRegion.stride = handleStride;
    out.missRegion.size = alignUp(missCount * handleStride, baseAlign);
    out.hitRegion.stride = handleStride;
    out.hitRegion.size = alignUp(hitCount * handleStride, baseAlign);
    // callableRegion stays all-zero (unused)

    // opaque handles, one per group, in group order
    std::vector<u8> handles(groups.size() * handleSize);
    VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(ctx.device, out.pipeline, 0, static_cast<u32>(groups.size()),
        handles.size(), handles.data()));


    // ai code
    // +baseAlign: VMA doesn't guarantee a 64-aligned address, so over-allocate and round the start up
    const VkDeviceSize sbtSize = out.rayGenRegion.size + out.missRegion.size + out.hitRegion.size + baseAlign;
    out.sbt = Buffer(ctx, sbtSize, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR, MemoryUsage::Upload, "SBT");

    const VkDeviceAddress base = alignUp(out.sbt.address(), baseAlign);
    const VkDeviceSize baseOffset = base - out.sbt.address();
    out.rayGenRegion.deviceAddress = base;
    out.missRegion.deviceAddress = base + out.rayGenRegion.size;
    out.hitRegion.deviceAddress = out.missRegion.deviceAddress + out.missRegion.size;

    u32 group = 0;
    auto writeRecord = [&](VkDeviceAddress regionAddress, u32 indexInRegion, VkDeviceSize stride) {
        const VkDeviceSize offset = baseOffset + (regionAddress - base) + indexInRegion * stride;
        out.sbt.upload(handles.data() + static_cast<size_t>(group++) * handleSize, handleSize, offset);
        };
    writeRecord(out.rayGenRegion.deviceAddress, 0, 0);
    for (u32 i = 0; i < missCount; ++i) writeRecord(out.missRegion.deviceAddress, i, handleStride);
    for (u32 i = 0; i < hitCount; ++i) writeRecord(out.hitRegion.deviceAddress, i, handleStride);

    if (ctx.caps.debugUtils) ctx.setName(out.pipeline, VK_OBJECT_TYPE_PIPELINE, "RayTracingPipeline");
    log::info("RT pipeline: {} groups, handle {} B, record stride {} B, SBT {} B", groups.size(), handleSize,
        handleStride, sbtSize);
    return out;
}


}