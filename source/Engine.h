#pragma once


#include <volk/volk.h>

#include <iostream>
#include <vector>
#include <deque>
#include <functional>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vma/vk_mem_alloc.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

struct DeletionQueue {
    std::deque<std::function<void()>> deletors;

    void push(std::function<void()>&& function)
    {
        deletors.push_back(function);
    }
    void flush()
    {
        for(auto it = deletors.rbegin(); it != deletors.rend(); ++it)
        {
            (*it)();
        }
        deletors.clear();
    }
};


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
    uint32_t queue_family_index;

    DeletionQueue main_deletion_queue;
    
    Engine();
    ~Engine();

    void physicalDeviceSelection();
    void logicalDeviceCreation();
    void queueFamilySelection(VkDeviceQueueCreateInfo* queue_create_info);
    void vmaSetup();
};
