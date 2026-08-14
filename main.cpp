#include "universe.h"

#include <GLFW/glfw3.h>

#include "PilsCore/Gpu/Instance.h"
#include "PilsCore/Gpu/Device.h"

#include "PilsCore/test/tests.h"

void initPilsLogger()
{
    pils::Logger::Settings logSettings;
    logSettings.outputToConsole = true;
    logSettings.outputToFile = true;
    logSettings.fileName = "unisim_log.txt";
#ifdef PILS_TARGET_DEBUG
    logSettings.level = pils::Logger::Debug;
#else
    logSettings.level = pils::Logger::Info;
#endif

    pils::Logger::getInstance().initialize(logSettings);
}

std::vector<const char*> getUserProvidedVkInstanceExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensionNames;
    for(uint32_t i = 0; i < glfwExtensionCount; ++i)
        extensionNames.push_back(glfwExtensions[i]);

    return extensionNames;
}

std::vector<const char*> getUserProvidedVkDeviceExtensions()
{
    std::vector<const char*> extensionNames;
    extensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    return extensionNames;
}

int main(int argc, char ** argv)
{
    initPilsLogger();
    bool allTestsPassed = runPilsCoreTests();

    if (!allTestsPassed)
    {
        PILS_ERROR("Some PilsCore tests failed, aborting UniSim");
        return -1;
    }

    glfwSetErrorCallback([](int error, const char* description)
    {
        PILS_ERROR("GLFW Error ", error, ": ",  description, "%s\n");
    });

    if (!glfwInit())
        return -1;

    pils::gpu::Instance::setUserExtensions(getUserProvidedVkInstanceExtensions());
    pils::gpu::Device::setUserExtensions(getUserProvidedVkDeviceExtensions());

    unisim::Universe universe;
    return universe.launch();
}
