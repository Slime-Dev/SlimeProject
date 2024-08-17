#include "PlatformerGame.h"

#include <DescriptorManager.h>
#include <ModelManager.h>
#include <SlimeWindow.h>
#include <spdlog/spdlog.h>
#include <VulkanContext.h>

PlatformerGame::PlatformerGame(SlimeWindow* window)
      : Scene()
{
}

int PlatformerGame::Enter(VulkanContext& vulkanContext, ModelManager& modelManager)
{
	return 0;
}

void PlatformerGame::Update(float dt, VulkanContext& vulkanContext, const InputManager* inputManager)
{

}

void PlatformerGame::Render()
{
}

void PlatformerGame::Exit(VulkanContext& vulkanContext, ModelManager& modelManager)
{
}