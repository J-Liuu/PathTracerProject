#pragma once

#include <functional>

#include "Common.h"

namespace pt::vk
{
	class Context;

	// records commands with `record`, submits them on ctx.queue and BLOCKS until the GPU is done.
	// for load-time work only (acceleration structure builds, uploads). Never call it every frame.
	void submitImmediate(const Context& ctx, const std::function<void(VkCommandBuffer)>& record);
}