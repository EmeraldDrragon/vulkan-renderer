#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#include <iostream>
#include <vector>
#include <array>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

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
struct shader_data
{
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model[3];
    glm::vec4 light_pos = {0.0f, -10.0f, 10.0f, 0.0f};
    uint32_t selected = 1;
} shader_data;
struct shader_data_buffer
{
    VmaAllocation allocation;
    VkBuffer buffer;
    VkDeviceAddress device_address;
    void* mapped;
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

    VmaAllocationCreateInfo buffer_alloc_info = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    vmaCreateBuffer(allocator, &buffer_create_info, &buffer_alloc_info, &v_buffer, &v_buffer_allocation, nullptr);

    void* buffer_ptr;
    vmaMapMemory(allocator, v_buffer_allocation, &buffer_ptr);
    memcpy(buffer_ptr, vertices.data(), v_buf_size);
    memcpy(((char*)buffer_ptr) + v_buf_size, indices.data(), i_buf_size);
    vmaUnmapMemory(allocator, v_buffer_allocation);

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
            .size = sizeof(shader_data),
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        };
        VmaAllocationCreateInfo u_buffer_alloc_info = {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        vmaCreateBuffer(allocator, &u_buffer_create_info, &u_buffer_alloc_info, &shader_data_buffers[i].buffer, &shader_data_buffers[i].allocation, nullptr);
        vmaMapMemory(allocator, shader_data_buffers[i].allocation, &shader_data_buffers[i].mapped);
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
        ktxTexture* ktx_texture;
        std::string filename = "assets/cat" + std::to_string(i)  + ".ktx2";
        ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);
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


    //cleanup
    for(auto& texture : textures)
    {
        vkDestroySampler(device, texture.sampler, nullptr);
        vkDestroyImageView(device, texture.view, nullptr);
        vmaDestroyImage(allocator, texture.image, texture.allocation);
    }
    for(auto i = 0; i < max_frames_in_flight; i++)
    {
        vkDestroySemaphore(device, present_semaphores[i], nullptr);
        vkDestroyFence(device, fences[i], nullptr);
    }
    for(auto semaphore : render_semaphores)
    {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    vkDestroyCommandPool(device, command_pool, nullptr);
    for(auto i = 0; i < max_frames_in_flight; i++)
    {
        vmaDestroyBuffer(allocator, shader_data_buffers[i].buffer, shader_data_buffers[i].allocation);
    }
    vmaDestroyBuffer(allocator, v_buffer, v_buffer_allocation);
    vkDestroyImageView(device, depth_image_view, nullptr);
    vmaDestroyImage(allocator, depth_image, depth_image_allocation);
    for (auto view : swapchain_image_views)
    {
        vkDestroyImageView(device, view, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vmaDestroyAllocator(allocator);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    vkDestroyInstance(instance, nullptr);
}