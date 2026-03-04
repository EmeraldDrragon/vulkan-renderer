#pragma once

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

class Engine
{

public:
    VkInstance instance;
    VkPhysicalDevice physical_device;
    SDL_Window* window;
    VkSurfaceKHR surface;
    VkDevice device;
    VkQueue queue;
    VkSurfaceCapabilitiesKHR surface_caps;
    VmaAllocator allocator;
    
    Engine();

    void physicalDeviceSelection();

    void logicalDeviceCreation();

    void queueFamilySelection(uint32_t* queue_family_index, VkDeviceQueueCreateInfo* queue_create_info);
    
    void vmaSetup();
};
