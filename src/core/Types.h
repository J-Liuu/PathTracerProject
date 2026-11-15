#pragma once
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// FILE FOR DEFINING GLOBAL TYPES
// feel free to add structs and types as seen fit
// - Michael

namespace pt
{
	using u8 = std::uint8_t;
	using u16 = std::uint16_t;
	using u32 = std::uint32_t;
	using u64 = std::uint64_t;
	using i32 = std::int32_t;
	using i64 = std::int64_t;

	using vec2 = glm::vec2;
	using vec3 = glm::vec3;
	using vec4 = glm::vec4;
	using uvec2 = glm::uvec2;
	using ivec2 = glm::ivec2;
	using mat3 = glm::mat3;
	using mat4 = glm::mat4;
	using quat = glm::quat;

	struct Extent2D
	{
		uint32_t width = 0;
		uint32_t height = 0;

		bool operator==(const Extent2D&) const = default;
		bool isZero() const { return width == 0 || height == 0; }
	};


	// READ THIS!!
	// Base for types that own GPU / OS resources.
	// Moveable
	// Never Copyable
	struct NonCopyable
	{
		NonCopyable() = default;
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;
		NonCopyable(NonCopyable&&) = default;
		NonCopyable& operator=(NonCopyable&&) = default;
	};
}