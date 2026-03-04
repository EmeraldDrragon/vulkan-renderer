#pragma once
#include <volk/volk.h>
#include "Engine.h"

class Output
{
private:
    VkFormat image_format;
    VkColorSpaceKHR image_color_space;
    VkSwapchainKHR swapchain;
    uint32_t image_count;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;

public:
    Output(Engine* engine)
    {
        getImageFormat(engine);
        createSwapchain(engine);
        createImageAndImageView(engine);
    }

    void getImageFormat(Engine* engine)
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

    void createSwapchain(Engine* engine)
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

    void createImageAndImageView(Engine* engine)
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

    void createDepthImageAndImageView(Engine* engine)
    {
        
    }

};