#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#include <iostream>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

int main()
{
    //init volk (Volk class)
    if(volkInitialize() != VK_SUCCESS)
    {
        std::cerr << "Vulkan Loader not initialized" << std::endl;
        return -1;
    }

    std::cout << "volk initialized" << std::endl;

    //init sdl (Sdl class)
    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    std::cout << "SDL initialized" << std::endl;

    //create window with vulkan flag (Sdl class)
    SDL_Window* window = SDL_CreateWindow("vulkan_render", 
        WINDOW_WIDTH, 
        WINDOW_HEIGHT, 
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if(!window)
    {
        std::cerr << "Window Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    std::cout << "SDL window created" << std::endl;

    //vulkan instance (VulkanEngine class)
    VkInstance instance;

    //vulkan validation layers (VulkanEngine class)
    std::vector<const char*> layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    //vulkan extensions (VulkanEngine class)
    uint32_t instance_extension_count = 0;
    char const* const* instance_extensions = SDL_Vulkan_GetInstanceExtensions(&instance_extension_count);

    //vulkan initialization (VulkanEngine class)
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "vulkan_render",
        .apiVersion = VK_API_VERSION_1_3
    };
    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = (uint32_t)layers.size(),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = instance_extension_count,
        .ppEnabledExtensionNames = instance_extensions
    };
    if(vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
    {
        std::cerr << "vulkan instance not created" << std::endl;
        return 1;
    }

    std::cout << "Vulkan instance created" << std::endl;

    //load instance to volk (Volk class)
    volkLoadInstance(instance);


    std::cout << "volk loaded" << std::endl;

    //Surface creation for SDL (Sdl class)
    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface);

    std::cout << "surface created" << std::endl;

    //physical device selection (VulkanEngine class)
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
    VkPhysicalDevice physical_device;
    for(const VkPhysicalDevice& device : devices)
    {
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(device, &device_properties);
        std::cout <<"Detected device: " << device_properties.deviceName << std::endl;
        if(device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            physical_device = device;
        }
        else if(device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            physical_device = device;
        }
    }
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(physical_device, &device_properties);
    std::cout << "selected device: " << device_properties.deviceName << std::endl;


    //queue family selection (VulkanEngine class)
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());
    uint32_t queue_family = 0;

    std::cout << "\n--- Queue Family Overview ---" << std::endl;

    for (size_t i = 0; i < queue_families.size(); i++) 
    {
        std::cout << "Family Index [" << i << "]" << std::endl;
        std::cout << "  Queue Count : " << queue_families[i].queueCount << std::endl;
        std::cout << "  Capabilities: ";

        // Check bits using the bitwise AND operator
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)       std::cout << "GRAPHICS ";
        if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)        std::cout << "COMPUTE ";
        if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)       std::cout << "TRANSFER ";
        if (queue_families[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) std::cout << "SPARSE ";
        
        // Video encoding/decoding (common on 3060)
        if (queue_families[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) std::cout << "VIDEO_DECODE ";
        
        std::cout << "\n  Timestamp Bits: " << queue_families[i].timestampValidBits << std::endl;
        std::cout << "-----------------------------" << std::endl;

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
        if((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present_support)
        {
            queue_family = i;
            break;
        }
    }
    std::cout << "chosen queue family index: " << queue_family << std::endl;

    //queue create info (VulkanEngine class)
    const float queue_priorities = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_priorities
    };

    //vulkan logical device (VulkanEngine class)
    VkDevice device;

    //device extensions (VulkanEngine class)
    const std::vector<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    //physical device features (VulkanEngine class)
    VkPhysicalDeviceVulkan12Features enabled_vk12_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = true,
        .shaderSampledImageArrayNonUniformIndexing = true,
        .descriptorBindingVariableDescriptorCount = true,
        .runtimeDescriptorArray = true,
        .bufferDeviceAddress = true
    };
    const VkPhysicalDeviceVulkan13Features enabled_vk13_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &enabled_vk12_features,
        .synchronization2 = true,
        .dynamicRendering = true,
    };
    const VkPhysicalDeviceFeatures enabled_vk10_features = {
        .samplerAnisotropy = VK_TRUE
    };

    //create logical device (VulkanEngine class)
    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &enabled_vk13_features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
        .pEnabledFeatures = &enabled_vk10_features,

    };
    if(vkCreateDevice(physical_device, &device_create_info, nullptr, &device) != VK_SUCCESS)
    {
        std::cerr << "vulkan logical device not created" << std::endl;
        return 1;
    }

    std::cout << "vulkan logical device created" << std::endl;

    //get queue to send commands to (VulkanEngine class)
    VkQueue queue;
    vkGetDeviceQueue(device, queue_family, 0, &queue);

    std::cout << "got graphics and presentation queue" << std::endl;

    //get surface capabilities from physical device (Sdl class)
    VkSurfaceCapabilitiesKHR surface_caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_caps);

    std::cout << "got surface capabilities from physical device" << std::endl;

    //VMA Setup (Vma class)
    VmaAllocator allocator;
    VmaVulkanFunctions vk_functions = {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        .vkCreateImage = vkCreateImage
    };
    VmaAllocatorCreateInfo allocator_info = {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = physical_device,
        .device = device,
        .pVulkanFunctions = &vk_functions,
        .instance = instance
    };
    vmaCreateAllocator(&allocator_info, &allocator);

   

    //Event loop temporary (Sdl calss)
    bool b_quit = false;
    SDL_Event event;
    while(!b_quit)
    {
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_EVENT_QUIT)
            {
                b_quit = true;
            }
            if(event.type == SDL_EVENT_KEY_DOWN)
            {
                if(event.key.key == SDLK_ESCAPE)
                {
                    b_quit = true;
                }
            }
        }
    }

    vmaDestroyAllocator(allocator);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    vkDestroyInstance(instance, nullptr);
}