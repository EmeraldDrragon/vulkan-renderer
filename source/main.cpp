#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <fstream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "slang/slang.h"
#include "slang/slang-com-ptr.h"

#include <ktx.h>
#include <ktxvulkan.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720


//Struct for vertex (Model class)
struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

//shader data (Renderer class)
struct shaderData
{
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model[3];
    glm::vec4 light_pos = {0.0f, -10.0f, 10.0f, 0.0f};
    uint32_t selected = 1;
};
struct shader_data_buffer
{
    VmaAllocation allocation;
    VmaAllocationInfo allocation_info;
    VkBuffer buffer;
    VkDeviceAddress device_address;
};

//textures (Model class)
struct texture
{
    VmaAllocation allocation;
    VkImage image;
    VkImageView view;
    VkSampler sampler;
};

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

    std::cout << "depth image and image view created" << std::endl;

    //Loading mesh (Model class)
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, "assets/Cat.obj");
    const VkDeviceSize index_count = shapes[0].mesh.indices.size();
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    for(auto& index : shapes[0].mesh.indices)
    {
        Vertex v = {
            .pos = {
                attrib.vertices[index.vertex_index * 3],
                -attrib.vertices[index.vertex_index * 3 + 1],
                attrib.vertices[index.vertex_index * 3 + 2]
            },
            .normal = {
                attrib.normals[index.normal_index * 3],
                -attrib.normals[index.normal_index * 3 + 1],
                attrib.normals[index.normal_index * 3 + 2]
            },
            .uv = {
                attrib.texcoords[index.texcoord_index * 2],
                1.0 - attrib.texcoords[index.texcoord_index * 2 + 1]
            }
        };
        vertices.push_back(v);
        indices.push_back(indices.size());
    }
    

    //upload mesh to gpu (Renderer class)
    VkBuffer v_buffer;
    VmaAllocation v_buffer_allocation;
    VkDeviceSize v_buf_size = sizeof(Vertex) * vertices.size();
    VkDeviceSize i_buf_size = sizeof(uint16_t) * indices.size();
    VkBufferCreateInfo buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = v_buf_size + i_buf_size,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    };

    VmaAllocationCreateInfo v_buffer_alloc_create_info = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    VmaAllocationInfo v_buffer_alloc_info;
    vmaCreateBuffer(allocator, &buffer_create_info, &v_buffer_alloc_create_info, &v_buffer, &v_buffer_allocation, &v_buffer_alloc_info);
    memcpy(v_buffer_alloc_info.pMappedData, vertices.data(), v_buf_size);
    memcpy(((char*)v_buffer_alloc_info.pMappedData) + v_buf_size, indices.data(), i_buf_size);

    std::cout << "mesh loaded into gpu" << std::endl;


    //cpu/gpu shared resources (Renderer class)
    constexpr uint32_t max_frames_in_flight = 2;
    std::array<shader_data_buffer, max_frames_in_flight> shader_data_buffers;
    std::array<VkCommandBuffer, max_frames_in_flight> command_buffers;

    //shader data buffers (Renderer class)
    for(auto i = 0; i < max_frames_in_flight; i ++)
    {
        VkBufferCreateInfo u_buffer_create_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(shaderData),
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        };
        VmaAllocationCreateInfo u_buffer_alloc_create_info = {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        vmaCreateBuffer(allocator, &u_buffer_create_info, &u_buffer_alloc_create_info, &shader_data_buffers[i].buffer, &shader_data_buffers[i].allocation, &shader_data_buffers[i].allocation_info);
        VkBufferDeviceAddressInfo u_buffer_device_address_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = shader_data_buffers[i].buffer
        };
        shader_data_buffers[i].device_address = vkGetBufferDeviceAddress(device, &u_buffer_device_address_info);
    }

    std::cout << "shared resources setup" << std::endl;


    //Synchronization objects (Renderer class)
    std::array<VkFence, max_frames_in_flight> fences;
    std::array<VkSemaphore, max_frames_in_flight> present_semaphores;
    std::vector<VkSemaphore> render_semaphores;
    VkSemaphoreCreateInfo semaphore_create_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    for(auto i = 0; i < max_frames_in_flight; i++)
    {
        vkCreateFence(device, &fence_create_info, nullptr, &fences[i]);
        vkCreateSemaphore(device, &semaphore_create_info, nullptr, &present_semaphores[i]);
    }
    render_semaphores.resize(swapchain_images.size());
    for(auto& semaphore : render_semaphores)
    {
        vkCreateSemaphore(device, &semaphore_create_info, nullptr, &semaphore);
    }

    std::cout << "synchronization objects created" << std::endl;


    //command buffers (Renderer class)
    VkCommandPool command_pool;
    VkCommandPoolCreateInfo command_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family
    };
    vkCreateCommandPool(device, &command_pool_create_info, nullptr, &command_pool);
    VkCommandBufferAllocateInfo command_buffer_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .commandBufferCount = max_frames_in_flight
    };
    vkAllocateCommandBuffers(device, &command_buffer_alloc_info, command_buffers.data());

    //loading textures (Model class)
    std::array<texture, 3> textures;
    std::vector<VkDescriptorImageInfo> texture_descriptors;
    for(auto i = 0; i < textures.size(); i++)
    {
        //creating texture image and image view (Model class)
        ktxTexture* ktx_texture = nullptr;
        std::string filename = "assets/cat" + std::to_string(i)  + ".ktx";
        KTX_error_code result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);
        if(result != KTX_SUCCESS || ktx_texture == nullptr)
        {
            std::cout << "could not load texture" << std::endl;
            return 1;
        }
        VkImageCreateInfo texture_image_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = ktxTexture_GetVkFormat(ktx_texture),
            .extent = {
                .width = ktx_texture->baseWidth,
                .height = ktx_texture->baseHeight,
                .depth = 1
            },
            .mipLevels = ktx_texture->numLevels,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };
        VmaAllocationCreateInfo texture_image_alloc_create_info = {
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        vmaCreateImage(allocator, &texture_image_create_info, &texture_image_alloc_create_info, &textures[i].image, &textures[i].allocation, nullptr);
        VkImageViewCreateInfo texture_image_view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = textures[i].image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = texture_image_create_info.format,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = ktx_texture->numLevels,
                .layerCount = 1
            }
        };
        vkCreateImageView(device, &texture_image_view_create_info, nullptr, &textures[i].view);

        //upload texture to gpu (Renderer class)
        VkBuffer image_source_buffer;
        VmaAllocation image_source_allocation;
        VkBufferCreateInfo image_source_buffer_create_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = (uint32_t)ktx_texture->dataSize,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        };
        VmaAllocationCreateInfo image_source_alloc_create_info = {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        VmaAllocationInfo image_source_alloc_info;
        vmaCreateBuffer(allocator, &image_source_buffer_create_info, &image_source_alloc_create_info, &image_source_buffer, &image_source_allocation, &image_source_alloc_info);
        memcpy(image_source_alloc_info.pMappedData, ktx_texture->pData, ktx_texture->dataSize);

        VkFenceCreateInfo fence_one_time_create_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
        };
        VkFence fence_one_time;
        vkCreateFence(device, &fence_one_time_create_info, nullptr, &fence_one_time);
        VkCommandBuffer command_buffer_one_time;
        VkCommandBufferAllocateInfo command_buffer_one_time_alloc_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = command_pool,
            .commandBufferCount = 1
        };
        vkAllocateCommandBuffers(device, &command_buffer_one_time_alloc_info, &command_buffer_one_time);

        VkCommandBufferBeginInfo command_buffer_one_time_begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        vkBeginCommandBuffer(command_buffer_one_time, &command_buffer_one_time_begin_info);
        VkImageMemoryBarrier2 barrier_texture_image = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .image = textures[i].image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = ktx_texture->numLevels,
                .layerCount = 1
            }
        };

        VkDependencyInfo barrier_texture_info = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier_texture_image
        };
        vkCmdPipelineBarrier2(command_buffer_one_time, &barrier_texture_info);
        std::vector<VkBufferImageCopy> copy_regions;
        for (auto j = 0; j < ktx_texture->numLevels; j++) 
        {
			ktx_size_t mipOffset{0};
			KTX_error_code ret = ktxTexture_GetImageOffset(ktx_texture, j, 0, 0, &mipOffset);
			copy_regions.push_back({
				.bufferOffset = mipOffset,
				.imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = (uint32_t)j,
                    .layerCount = 1
                },
				.imageExtent = {
                    .width = ktx_texture->baseWidth >> j,
                    .height = ktx_texture->baseHeight >> j,
                    .depth = 1
                },
			});
		}
        vkCmdCopyBufferToImage(command_buffer_one_time, image_source_buffer, textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copy_regions.size()), copy_regions.data());
        VkImageMemoryBarrier2 barrier_texture_read = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
			.image = textures[i].image,
			.subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = ktx_texture->numLevels,
                .layerCount = 1
            }
		};
        barrier_texture_info.pImageMemoryBarriers = &barrier_texture_read;
        vkCmdPipelineBarrier2(command_buffer_one_time, &barrier_texture_info);
        vkEndCommandBuffer(command_buffer_one_time);
        VkSubmitInfo one_time_submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer_one_time
        };
        vkQueueSubmit(queue, 1, &one_time_submit_info, fence_one_time);
        vkWaitForFences(device, 1, &fence_one_time, VK_TRUE, UINT64_MAX);
        vkDestroyFence(device, fence_one_time, nullptr);
        vmaDestroyBuffer(allocator, image_source_buffer, image_source_allocation);

        //sampler (Renderer class)
        VkSamplerCreateInfo sampler_create_info = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = 8.0f,
            .maxLod = (float)ktx_texture->numLevels
        };
        vkCreateSampler(device, &sampler_create_info, nullptr, &textures[i].sampler);
        ktxTexture_Destroy(ktx_texture);
        texture_descriptors.push_back({
            .sampler = textures[i].sampler,
            .imageView = textures[i].view,
            .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
        });
    }

    std::cout << "textures loaded" << std::endl;

    //Descriptors (Renderer class)
    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout descriptor_set_layout_textures;
    VkDescriptorSet descriptor_set_textures;
    VkDescriptorBindingFlags desc_var_flag = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
    VkDescriptorSetLayoutBindingFlagsCreateInfo desc_binding_flags = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = 1,
        .pBindingFlags = &desc_var_flag
    };
    VkDescriptorSetLayoutBinding desc_layout_binding_textures = {
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(textures.size()),
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };
    VkDescriptorSetLayoutCreateInfo desc_layout_texture_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &desc_binding_flags,
        .bindingCount = 1,
        .pBindings = &desc_layout_binding_textures
    };
    vkCreateDescriptorSetLayout(device, &desc_layout_texture_create_info, nullptr, &descriptor_set_layout_textures);
    
    VkDescriptorPoolSize pool_size = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(textures.size())
    };
    VkDescriptorPoolCreateInfo desc_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &pool_size
    };
    vkCreateDescriptorPool(device, &desc_pool_create_info, nullptr, &descriptor_pool);

    uint32_t var_desc_count = static_cast<uint32_t>(textures.size());
    VkDescriptorSetVariableDescriptorCountAllocateInfo var_desc_count_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
        .descriptorSetCount = 1,
        .pDescriptorCounts = &var_desc_count
    };
    VkDescriptorSetAllocateInfo texture_desc_set_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = &var_desc_count_alloc_info,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptor_set_layout_textures
    };
    vkAllocateDescriptorSets(device, &texture_desc_set_alloc_info, &descriptor_set_textures);

    VkWriteDescriptorSet write_desc_set = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set_textures,
        .dstBinding = 0,
        .descriptorCount = static_cast<uint32_t>(texture_descriptors.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = texture_descriptors.data()
    };
    vkUpdateDescriptorSets(device, 1, &write_desc_set, 0, nullptr);

    std::cout << "setup descriptors" << std::endl;

    //shaders (Renderer class)
    Slang::ComPtr<slang::IGlobalSession> slang_global_session;
    slang::createGlobalSession(slang_global_session.writeRef());
    auto slang_targets = std::to_array<slang::TargetDesc>({{
        .format = SLANG_SPIRV,
        .profile = slang_global_session->findProfile("spirv_1_4")
    }});
    auto slang_options = std::to_array<slang::CompilerOptionEntry>({{slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1}}});
    slang::SessionDesc slang_session_desc = {
        .targets = slang_targets.data(),
        .targetCount =  SlangInt(slang_targets.size()),
        .defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
        .compilerOptionEntries = slang_options.data(),
        .compilerOptionEntryCount = uint32_t(slang_options.size())
    };
    Slang::ComPtr<slang::ISession> slang_session;
    slang_global_session->createSession(slang_session_desc, slang_session.writeRef());
    Slang::ComPtr<slang::IModule> slang_module{
        slang_session->loadModuleFromSource("triangle", "assets/shader.slang", nullptr, nullptr)
    };
    Slang::ComPtr<ISlangBlob> spirv;
    slang_module->getTargetCode(0, spirv.writeRef());
    VkShaderModuleCreateInfo shader_module_create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv->getBufferSize(),
        .pCode = (uint32_t*)spirv->getBufferPointer()
    };
    VkShaderModule shader_module;
    vkCreateShaderModule(device, &shader_module_create_info, nullptr, &shader_module);

    std::cout << "shader loaded" << std::endl;


    //Graphics pipeline (Renderer class)
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkPushConstantRange push_constant_range = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .size = sizeof(VkDeviceAddress)
        };
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_set_layout_textures,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constant_range
    };
    vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipeline_layout);
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages{
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = shader_module,
            .pName = "main"
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = shader_module,
            .pName = "main"
        }
    };
    VkVertexInputBindingDescription vertex_binding = {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    std::vector<VkVertexInputAttributeDescription> vertex_attributes = {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, normal)
        },
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(Vertex, uv)
        }
    };
    VkPipelineVertexInputStateCreateInfo vertex_input_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_binding,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attributes.size()),
        .pVertexAttributeDescriptions = vertex_attributes.data()
    };
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates.data()
    };
    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };
    VkPipelineRasterizationStateCreateInfo rasterization_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .lineWidth = 1.0f
    };
    VkPipelineMultisampleStateCreateInfo multisample_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
    };
    VkPipelineColorBlendAttachmentState blend_attachment = { 
        .colorWriteMask = 0xF
    };
    VkPipelineColorBlendStateCreateInfo color_blend_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blend_attachment
    };
    VkPipelineRenderingCreateInfo rendering_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1, .pColorAttachmentFormats = &image_format,
        .depthAttachmentFormat = depth_format
    };
    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &rendering_create_info,
        .stageCount = 2,
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_input_state,
        .pInputAssemblyState = &input_assembly_state,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterization_state,
        .pMultisampleState = &multisample_state,
        .pDepthStencilState = &depth_stencil_state,
        .pColorBlendState= &color_blend_state,
        .pDynamicState = &dynamic_state,
        .layout = pipeline_layout
    };
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline);
	
    std::cout << "graphics pipeline created" << std::endl;
    std::cout << "starting render loop" << std::endl;

    //Render loop (Renderer class)
    bool update_swapchain = false;
    shaderData shader_data;
    uint32_t image_index = 0;
    uint32_t frame_index = 0;
    uint64_t last_time = SDL_GetTicks();
    glm::vec3 cam_pos = {0.0f, 0.0f, -0.0f};
    glm::vec3 object_rotations[3]{};
    bool quit = false;
    while(!quit)
    {
        vkWaitForFences(device, 1, &fences[frame_index], true, UINT64_MAX);
        vkResetFences(device, 1, &fences[frame_index]);
        vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, present_semaphores[frame_index], VK_NULL_HANDLE, &image_index);

        shader_data.projection = glm::perspective(glm::radians(45.0f), (float)window_width / (float)window_height, 0.1f, 32.0f);
        shader_data.view = glm::translate(glm::mat4(1.0f), cam_pos);
        for(auto i = 0; i < 3; i++)
        {
            auto instance_pos = glm::vec3((float)(i - 1) * 3.0f, 0.0f, 0.0f);
            shader_data.model[i] = glm::translate(glm::mat4(1.0f), instance_pos) * glm::mat4_cast(glm::quat(object_rotations[i]));
        }
		memcpy(shader_data_buffers[frame_index].allocation_info.pMappedData, &shader_data, sizeof(shaderData));

        auto command_buffer = command_buffers[frame_index];
        VkCommandBufferBeginInfo command_buffer_begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);
        std::array<VkImageMemoryBarrier2, 2> output_barriers = {
            VkImageMemoryBarrier2{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = 0,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
				.image = swapchain_images[image_index],
				.subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
			},
			VkImageMemoryBarrier2{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
				.image = depth_image,
				.subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .levelCount = 1, .layerCount = 1 }
			}
        };
        VkDependencyInfo barrier_dependency_info = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 2,
            .pImageMemoryBarriers = output_barriers.data()
        };
		vkCmdPipelineBarrier2(command_buffer, &barrier_dependency_info);
        VkRenderingAttachmentInfo color_attachment_info = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = swapchain_image_views[image_index],
			.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue{
                .color{ 0.0f, 0.0f, 0.0f, 1.0f }
            }
		};
		VkRenderingAttachmentInfo depth_attachment_info = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = depth_image_view,
			.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.clearValue = {
                .depthStencil = {1.0f,  0}
            }
		};
		VkRenderingInfo rendering_info = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = {
                .extent = {
                    .width = static_cast<uint32_t>(window_width),
                    .height = static_cast<uint32_t>(window_height)
                }
            },
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment_info,
			.pDepthAttachment = &depth_attachment_info
		};
        vkCmdBeginRendering(command_buffer, &rendering_info);
        VkViewport viewport = {
            .width = static_cast<float>(window_width),
            .height = static_cast<float>(window_height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
		vkCmdSetViewport(command_buffer, 0, 1, &viewport);
		VkRect2D scissor = {
            .extent = {
                .width = static_cast<uint32_t>(window_width),
                .height = static_cast<uint32_t>(window_height)
            }
        };
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);
		vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set_textures, 0, nullptr);
		VkDeviceSize v_offset = 0;
		vkCmdBindVertexBuffers(command_buffer, 0, 1, &v_buffer, &v_offset);
		vkCmdBindIndexBuffer(command_buffer, v_buffer, v_buf_size, VK_INDEX_TYPE_UINT16);
		vkCmdPushConstants(command_buffer, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &shader_data_buffers[frame_index].device_address);
		vkCmdDrawIndexed(command_buffer, index_count, 3, 0, 0, 0);
		vkCmdEndRendering(command_buffer);
		VkImageMemoryBarrier2 barrier_present = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstAccessMask = 0,
			.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.image = swapchain_images[image_index],
			.subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
		};
		VkDependencyInfo barrier_present_dependency_info = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier_present
        };
		vkCmdPipelineBarrier2(command_buffer, &barrier_present_dependency_info);
		vkEndCommandBuffer(command_buffer);

        VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submit_info = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &present_semaphores[frame_index],
			.pWaitDstStageMask = &wait_stages,
			.commandBufferCount = 1,
			.pCommandBuffers = &command_buffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &render_semaphores[image_index],
		};
		vkQueueSubmit(queue, 1, &submit_info, fences[frame_index]);
		frame_index = (frame_index + 1) % max_frames_in_flight;

		VkPresentInfoKHR presentInfo{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &render_semaphores[image_index],
			.swapchainCount = 1,
			.pSwapchains = &swapchain,
			.pImageIndices = &image_index
		};
		vkQueuePresentKHR(queue, &presentInfo);

        float elapsed_time{ (SDL_GetTicks() - last_time) / 1000.0f };
		last_time = SDL_GetTicks();
		for (SDL_Event event; SDL_PollEvent(&event);)
        {
			if (event.type == SDL_EVENT_QUIT)
            {
				quit = true;
				break;
			}
			if (event.type == SDL_EVENT_MOUSE_MOTION)
            {
				if (event.button.button == SDL_BUTTON_LEFT)
                {
					object_rotations[shader_data.selected].x -= (float)event.motion.yrel * elapsed_time;
					object_rotations[shader_data.selected].y += (float)event.motion.xrel * elapsed_time;
				}
			}
			if (event.type == SDL_EVENT_MOUSE_WHEEL)
            {
				cam_pos.z += (float)event.wheel.y * elapsed_time * 10.0f;
			}
			if (event.type == SDL_EVENT_KEY_DOWN)
            {
				if (event.key.key == SDLK_PLUS || event.key.key == SDLK_KP_PLUS)
                {
					shader_data.selected = (shader_data.selected < 2) ? shader_data.selected + 1 : 0;
				}
				if (event.key.key == SDLK_MINUS || event.key.key == SDLK_KP_MINUS)
                {
					shader_data.selected = (shader_data.selected > 0) ? shader_data.selected - 1 : 2;
				}
			}
			if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
				update_swapchain = true;
			}
		}
        if (update_swapchain)
        {
			SDL_GetWindowSize(window, &window_width, &window_height);
			update_swapchain = false;
			vkDeviceWaitIdle(device);
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_caps);
			swapchain_create_info.oldSwapchain = swapchain;
			swapchain_create_info.imageExtent = {
                .width = static_cast<uint32_t>(window_width),
                .height = static_cast<uint32_t>(window_height)
            };
			vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain);
			for (auto i = 0; i < image_count; i++)
            {
				vkDestroyImageView(device, swapchain_image_views[i], nullptr);
			}
			vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
			swapchain_images.resize(image_count);
			vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images.data());
			swapchain_image_views.resize(image_count);
			for (auto i = 0; i < image_count; i++)
            {
				VkImageViewCreateInfo view_create_info = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = swapchain_images[i], .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = image_format,
                    .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .levelCount = 1,
                        .layerCount = 1
                    }
                };
				vkCreateImageView(device, &view_create_info, nullptr, &swapchain_image_views[i]);
			}
			vkDestroySwapchainKHR(device, swapchain_create_info.oldSwapchain, nullptr);
			vmaDestroyImage(allocator, depth_image, depth_image_allocation);
			vkDestroyImageView(device, depth_image_view, nullptr);
			depth_image_create_info.extent = {
                .width = static_cast<uint32_t>(window_width),
                .height = static_cast<uint32_t>(window_height),
                .depth = 1
            };
			VmaAllocationCreateInfo alloc_create_info = {
                .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO
            };
			vmaCreateImage(allocator, &depth_image_create_info, &alloc_create_info, &depth_image, &depth_image_allocation, nullptr);
			VkImageViewCreateInfo view_create_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = depth_image, .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = depth_format,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .levelCount = 1,
                    .layerCount = 1
                }
            };
			vkCreateImageView(device, &view_create_info, nullptr, &depth_image_view);
		}
    }

    std::cout << "render loop finished" << std::endl;
	
	

    





    //cleanup
	vkDeviceWaitIdle(device);
	for (auto i = 0; i < max_frames_in_flight; i++) {
		vkDestroyFence(device, fences[i], nullptr);
		vkDestroySemaphore(device, present_semaphores[i], nullptr);
		vmaDestroyBuffer(allocator, shader_data_buffers[i].buffer, shader_data_buffers[i].allocation);
	}
	for (auto i = 0; i < render_semaphores.size(); i++) {
		vkDestroySemaphore(device, render_semaphores[i], nullptr);
	}
	vmaDestroyImage(allocator, depth_image, depth_image_allocation);
	vkDestroyImageView(device, depth_image_view, nullptr);
	for (auto i = 0; i < swapchain_image_views.size(); i++) {
		vkDestroyImageView(device, swapchain_image_views[i], nullptr);
	}
	vmaDestroyBuffer(allocator, v_buffer, v_buffer_allocation);
	for (auto i = 0; i < textures.size(); i++) {
		vkDestroyImageView(device, textures[i].view, nullptr);
		vkDestroySampler(device, textures[i].sampler, nullptr);
		vmaDestroyImage(allocator, textures[i].image, textures[i].allocation);
	}
	vkDestroyDescriptorSetLayout(device, descriptor_set_layout_textures, nullptr);
	vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
	vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyCommandPool(device, command_pool, nullptr);
	vkDestroyShaderModule(device, shader_module, nullptr);
	vmaDestroyAllocator(allocator);
	SDL_DestroyWindow(window);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_Quit();
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);
}