#include "Swapchain.h"

#include <algorithm>

#include "Context.h"

namespace pt::vk {

Swapchain::Swapchain(const Context& ctx, Extent2D extent, bool vsync) : m_ctx(ctx), m_vsync(vsync) {
    create(extent, VK_NULL_HANDLE);
}

Swapchain::~Swapchain() {
    destroyViews();
    if (m_swapchain) vkDestroySwapchainKHR(m_ctx.device, m_swapchain, nullptr);
}

void Swapchain::recreate(Extent2D extent) {
    m_ctx.waitIdle();
    destroyViews();
    VkSwapchainKHR old = m_swapchain;
    create(extent, old);
    vkDestroySwapchainKHR(m_ctx.device, old, nullptr);
}

void Swapchain::create(Extent2D extent, VkSwapchainKHR old) {
    VkSurfaceCapabilitiesKHR caps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_ctx.physicalDevice, m_ctx.surface, &caps));

    // Format: UNORM so the tonemap compute pass can write it as a storage image;
    // the shader does the sRGB encode.
    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_ctx.physicalDevice, m_ctx.surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_ctx.physicalDevice, m_ctx.surface, &formatCount, formats.data());
    VkSurfaceFormatKHR chosen = formats[0];
    for (const auto& f : formats) {
        if ((f.format == VK_FORMAT_B8G8R8A8_UNORM || f.format == VK_FORMAT_R8G8B8A8_UNORM) &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen = f;
            break;
        }
    }
    m_format = chosen.format;

    // Present mode
    u32 modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_ctx.physicalDevice, m_ctx.surface, &modeCount, nullptr);
    std::vector<VkPresentModeKHR> modes(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_ctx.physicalDevice, m_ctx.surface, &modeCount, modes.data());
    VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;  // always available
    if (!m_vsync) {
        for (VkPresentModeKHR m : {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR}) {
            if (std::find(modes.begin(), modes.end(), m) != modes.end()) {
                mode = m;
                break;
            }
        }
    }

    if (caps.currentExtent.width != ~0u) {
        m_extent = { caps.currentExtent.width, caps.currentExtent.height };
    }
    else {
        m_extent = { std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width),
                    std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height) };
    }

    u32 imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0) imageCount = std::min(imageCount, caps.maxImageCount);

    VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (caps.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT) usage |= VK_IMAGE_USAGE_STORAGE_BIT;

    VkSwapchainCreateInfoKHR info{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    info.surface = m_ctx.surface;
    info.minImageCount = imageCount;
    info.imageFormat = chosen.format;
    info.imageColorSpace = chosen.colorSpace;
    info.imageExtent = { m_extent.width, m_extent.height };
    info.imageArrayLayers = 1;
    info.imageUsage = usage;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = caps.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = mode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = old;
    VK_CHECK(vkCreateSwapchainKHR(m_ctx.device, &info, nullptr, &m_swapchain));

    u32 count = 0;
    vkGetSwapchainImagesKHR(m_ctx.device, m_swapchain, &count, nullptr);
    m_images.resize(count);
    vkGetSwapchainImagesKHR(m_ctx.device, m_swapchain, &count, m_images.data());

    m_views.resize(count);
    m_renderFinished.resize(count);
    for (u32 i = 0; i < count; ++i) {
        VkImageViewCreateInfo v{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        v.image = m_images[i];
        v.viewType = VK_IMAGE_VIEW_TYPE_2D;
        v.format = m_format;
        v.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        VK_CHECK(vkCreateImageView(m_ctx.device, &v, nullptr, &m_views[i]));

        VkSemaphoreCreateInfo s{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VK_CHECK(vkCreateSemaphore(m_ctx.device, &s, nullptr, &m_renderFinished[i]));
    }
}

void Swapchain::destroyViews() {
    for (VkImageView v : m_views) vkDestroyImageView(m_ctx.device, v, nullptr);
    for (VkSemaphore s : m_renderFinished) vkDestroySemaphore(m_ctx.device, s, nullptr);
    m_views.clear();
    m_renderFinished.clear();
}

bool Swapchain::acquire(VkSemaphore imageAvailable, u32& outImageIndex) {
    const VkResult r = vkAcquireNextImageKHR(m_ctx.device, m_swapchain, UINT64_MAX, imageAvailable,
        VK_NULL_HANDLE, &outImageIndex);
    if (r == VK_ERROR_OUT_OF_DATE_KHR) return false;
    PT_ASSERT(r == VK_SUCCESS || r == VK_SUBOPTIMAL_KHR, "vkAcquireNextImageKHR failed");
    return true;
}

bool Swapchain::present(u32 imageIndex) {
    VkSemaphore wait = m_renderFinished[imageIndex];
    VkPresentInfoKHR info{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &wait;
    info.swapchainCount = 1;
    info.pSwapchains = &m_swapchain;
    info.pImageIndices = &imageIndex;
    const VkResult r = vkQueuePresentKHR(m_ctx.queue, &info);
    if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) return false;
    PT_ASSERT(r == VK_SUCCESS, "vkQueuePresentKHR failed");
    return true;
    }

}