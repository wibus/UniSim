#ifdef UNISIM_GRAPHIC_BACKEND_VK

#include "imguirenderer_vk.h"
#include "imguirenderer.h"

#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include <PilsCore/Utils/Assert.h>

#include <PilsCore/Gpu/Context.h>
#include <PilsCore/Gpu/Surface.h>
#include <PilsCore/Gpu/Pass.h>
#include <PilsCore/Gpu/Command.h>

#include "window.h"
#include "view.h"
#include "graphic.h"


namespace unisim
{

ImGuiNativeData::ImGuiNativeData(const pils::gpu::Device& device) :
    _device(device),
    _descriptorPool{}
{
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE },
            { VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE },
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 0;
        for (VkDescriptorPoolSize& pool_size : pool_sizes)
            pool_info.maxSets += pool_size.descriptorCount;
        pool_info.poolSizeCount = (uint32_t)IM_COUNTOF(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        VkResult result = vkCreateDescriptorPool(_device.vkDevice(), &pool_info, nullptr, &_descriptorPool);

        PILS_ASSERT(result == VkResult::VK_SUCCESS, "Failed to create ImGui descriptor pool");
    }
}

ImGuiNativeData::~ImGuiNativeData()
{
    ImGui_ImplVulkan_Shutdown();

    vkDestroyDescriptorPool(_device.vkDevice(), _descriptorPool, nullptr);
}

static void check_vk_result(VkResult err)
{
    PILS_ASSERT(err == VK_SUCCESS, "Failed to initialize ImGui for Vulkan");
}

void ImGuiRenderer::ImGuiInitNative()
{
    const Window& window = _view.window();
    const pils::gpu::Context& context = window.gpuDevice().context();

    _imguiNativeData.reset(new ImGuiNativeData(context.device()));

    ImGui_ImplGlfw_InitForVulkan(window.glfwWindow(), true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    //init_info.ApiVersion = VK_API_VERSION_1_3;              // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
    init_info.Instance = context.instance().vkInstance();
    init_info.PhysicalDevice = context.physicalDevice().vkPhysicalDevice();
    init_info.Device = context.device().vkDevice();
    init_info.QueueFamily = context.device().queueFamily();
    init_info.Queue = context.device().vkQueue();
    init_info.PipelineCache = context.device().vkPipelineCache();
    init_info.DescriptorPool = _imguiNativeData->descriptorPool();
    init_info.MinImageCount = window.swapchain().imageCount(); // get exact minImageCount from vk?
    init_info.ImageCount = window.swapchain().imageCount();
    init_info.Allocator = nullptr;
    init_info.PipelineInfoMain.RenderPass = _view.renderPass().vkRenderPass();
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = check_vk_result;
    bool ok = ImGui_ImplVulkan_Init(&init_info);

    if (!ok)
        PILS_ERROR("Could not initialize ImGui for Vulkan");
}

void ImGuiRenderer::ImGuiNewFrameNative() const
{
    ImGui_ImplVulkan_NewFrame();
}

void ImGuiRenderer::renderNative(GpuDevice& gpuDevice, const View& view) const
{
    gpuDevice.commandList().beginRenderPass(view.renderPass(), view.frameBuffer());
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), gpuDevice.commandList().vkCommandBuffer());
    gpuDevice.commandList().endRenderPass();
}

}

#endif // UNISIM_GRAPHIC_BACKEND_VK