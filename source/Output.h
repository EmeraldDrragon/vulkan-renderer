#pragma once

#include <volk/volk.h>
#include "Engine.h"
#include "ImageAlloc.h"

class Output
{
public:
    VkFormat image_format;
    VkColorSpaceKHR image_color_space;
    VkSwapchainKHR swapchain;
    uint32_t image_count;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    ImageAlloc depth_attachment;
    VkFormat depth_format;
    int window_width;
    int window_height;

    Output(Engine* engine)
    {
        getImageFormat(engine);
        createSwapchain(engine);
        createImageAndImageView(engine);
        SDL_GetWindowSize(engine->window, &window_width, &window_height);
        createDepthImageAndImageView(engine);
    }

    void getImageFormat(Engine* engine);
    void createSwapchain(Engine* engine);
    void createImageAndImageView(Engine* engine);
    void createDepthImageAndImageView(Engine* engine);
};