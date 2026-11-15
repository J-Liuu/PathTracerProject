#include <scene/Scene.h>

namespace pt
{
u32 Scene::addMaterial(Material m)
{
	materials.push_back(std::move(m));
	markDirty(DirtyMaterials);
	return static_cast<u32>(materials.size() - 1);
}

u32 Scene::addMesh(Mesh m)
{
	meshes.push_back(std::move(m));
	markDirty(DirtyGeometry);
	return static_cast<u32>(meshes.size() - 1);
}

void Scene::addInstance(u32 meshIndex, const mat4& transform)
{
	instances.push_back({ meshIndex, transform });
	markDirty(DirtyInstances);
}
}