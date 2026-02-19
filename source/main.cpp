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

    //init sdl (Output class)
    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    std::cout << "SDL initialized" << std::endl;

    //create window with vulkan flag (Output class)
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

    //Surface creation for SDL (Output class)
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
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)       std::cout << "GRAPHICS ";
        if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)        std::cout << "COMPUTE ";
        if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)       std::cout << "TRANSFER ";
        if (queue_families[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) std::cout << "SPARSE ";
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

    //get surface capabilities from physical device (Output class)
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


    //get image format (Output class)
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats.data());
    VkSurfaceFormatKHR surface_format = formats.data()[0];
    for(VkSurfaceFormatKHR available_format : formats)
    {
        if(available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && 
            available_format.format == VK_FORMAT_B8G8R8A8_SRGB)
        {
            surface_format = available_format;
            break;  
        }
    }
    VkFormat image_format = surface_format.format;
    std::cout << "got image color format" << std::endl;
    VkColorSpaceKHR image_color_space = surface_format.colorSpace;
    std::cout << "got image color space" << std::endl;
    
    //Swapchain (Output class)
    VkSwapchainKHR swapchain;
    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = surface_caps.minImageCount,
        .imageFormat = image_format,
        .imageColorSpace = image_color_space,
        .imageExtent = {
            .width = surface_caps.currentExtent.width,
            .height = surface_caps.currentExtent.height
        },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = surface_caps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR
    };
    
    if(vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain) != VK_SUCCESS)
    {
        std::cerr << "swapchain not created" << std::endl;
        return 1;
    }

    std::cout << "swapchain created" << std::endl;

    //create image and imageview (Output class)
    uint32_t image_count = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
    std::vector<VkImage> swapchain_images(image_count);
    std::vector<VkImageView> swapchain_image_views(image_count);
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images.data());

    std::cout << "got swapchain images" << std::endl;

    for(auto i = 0; i < image_count; i++)
    {
        VkImageViewCreateInfo image_view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = image_format,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            }
        };
        vkCreateImageView(device, &image_view_create_info, nullptr, &swapchain_image_views[i]);
    }

    std::cout << "created image views" << std::endl;

    //create depth attachment (Output class)
    VkImage depth_image;
    VmaAllocation depth_image_allocation;
    VkImageView depth_image_view;
    std::vector<VkFormat> depthFormatList = { 
        VK_FORMAT_D32_SFLOAT_S8_UINT, 
        VK_FORMAT_D24_UNORM_S8_UINT 
    };
    VkFormat depth_format = VK_FORMAT_UNDEFINED;
    for(VkFormat& format : depthFormatList)
    {
        VkFormatProperties2 format_properties = {
            .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2
        };
        vkGetPhysicalDeviceFormatProperties2(physical_device, format, &format_properties);
        if (format_properties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) 
        {
			depth_format = format;
			break;
		}
    }
    int window_width;
    int window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);
    VkImageCreateInfo depth_image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depth_format,
        .extent = {
            .width = static_cast<uint32_t>(window_width), 
            .height = static_cast<uint32_t>(window_height), 
            .depth = 1 
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    VmaAllocationCreateInfo alloc_create_info = {
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    vmaCreateImage(allocator, &depth_image_create_info, &alloc_create_info, &depth_image, &depth_image_allocation, nullptr);
    VkImageViewCreateInfo depth_image_view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = depth_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depth_format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };
    vkCreateImageView(device, &depth_image_view_create_info, nullptr, &depth_image_view);


    //Event loop temporary (Output class)
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

    for (auto view : swapchain_image_views)
    {
        vkDestroyImageView(device, view, nullptr);
    }
    vkDestroyImageView(device, depth_image_view, nullptr);
    vmaDestroyImage(allocator, depth_image, depth_image_allocation);
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vmaDestroyAllocator(allocator);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    vkDestroyInstance(instance, nullptr);
}