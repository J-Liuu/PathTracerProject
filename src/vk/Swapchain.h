#pragma once

#include <vector>

#include "Common.h"

namespace pt::vk {

class Context;

class Swapchain : NonCopyable {
public:
    Swapchain(const Context& ctx, Extent2D extent, bool vsync);
    ~Swapchain();

    void recreate(Extent2D extent);

    // Returns false if the swapchain is out of date. (when you resize window) (gets reccreated).
    bool acquire(VkSemaphore imageAvailable, u32& outImageIndex);
    bool present(u32 imageIndex);

    VkImage image(u32 i) const { return m_images[i]; }
    VkImageView view(u32 i) const { return m_views[i]; }
   
    // render finished semaphore per swapcahin image
    VkSemaphore renderFinished(u32 i) const { return m_renderFinished[i]; }
    VkFormat format() const { return m_format; }
    Extent2D extent() const { return m_extent; }
    u32 imageCount() const { return static_cast<u32>(m_images.size()); }

private:
    void create(Extent2D extent, VkSwapchainKHR old);
    void destroyViews();

    const Context& m_ctx;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_views;
    std::vector<VkSemaphore> m_renderFinished;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    Extent2D m_extent{};
    bool m_vsync = true;
};

}