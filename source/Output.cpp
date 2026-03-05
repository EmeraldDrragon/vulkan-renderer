    #include "Output.h"
    
    Output::Output(Engine* engine)
    {
        getImageFormat(engine);
        createSwapchain(engine);
        createImageAndImageView(engine);
        SDL_GetWindowSize(engine->window, &window_width, &window_height);
        createDepthImageAndImageView(engine);
    }

    void Output::getImageFormat(Engine* engine)
    {
        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(engine->physical_device, engine->surface, &format_count, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(engine->physical_device, engine->surface, &format_count, formats.data());
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
        image_format = surface_format.format;
        std::cout << "got image color format" << std::endl;
        image_color_space = surface_format.colorSpace;
        std::cout << "got image color space" << std::endl;
    }

    void Output::createSwapchain(Engine* engine)
    {
        VkSwapchainCreateInfoKHR swapchain_create_info = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = engine->surface,
            .minImageCount = engine->surface_caps.minImageCount,
            .imageFormat = image_format,
            .imageColorSpace = image_color_space,
            .imageExtent = {
                .width = engine->surface_caps.currentExtent.width,
                .height = engine->surface_caps.currentExtent.height
            },
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .preTransform = engine->surface_caps.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = VK_PRESENT_MODE_FIFO_KHR
        };
        vkCreateSwapchainKHR(engine->device, &swapchain_create_info, nullptr, &swapchain);
        std::cout << "swapchain created" << std::endl;
    }

    void Output::createImageAndImageView(Engine* engine)
    {
        image_count = 0;
        vkGetSwapchainImagesKHR(engine->device, swapchain, &image_count, nullptr);
        swapchain_images.resize(image_count);
        swapchain_image_views.resize(image_count);
        vkGetSwapchainImagesKHR(engine->device, swapchain, &image_count, swapchain_images.data());

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
            vkCreateImageView(engine->device, &image_view_create_info, nullptr, &swapchain_image_views[i]);
        }

        std::cout << "created image views" << std::endl;
    }

    void Output::createDepthImageAndImageView(Engine* engine)
    {
        std::vector<VkFormat> depth_format_list = {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };
        for(VkFormat& format : depth_format_list)
        {
            VkFormatProperties2 format_properties = {
                .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2
            };
            vkGetPhysicalDeviceFormatProperties2(engine->physical_device, format, &format_properties);
            if(format_properties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                depth_format = format;
                break;
            }
        }
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
        VmaAllocationCreateFlags depth_vma_flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        VkImageAspectFlags depth_aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depth_attachment = ImageAlloc::create(engine->allocator, engine->device, depth_image_create_info, depth_vma_flags, depth_aspect_mask);
        
        //add here addition to deletion queue

        std::cout << "depth image and image view created" << std::endl;
    }