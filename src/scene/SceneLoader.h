#pragma once

#include <filesystem>
#include <optional>

#include <scene/Scene.h>

namespace pt
{
// procedural cornell box for lightweight reference scene (ai generated)
Scene makeCornellBox();

std::optional<Scene> loadGltf(const std::filesystem::path& path);
}