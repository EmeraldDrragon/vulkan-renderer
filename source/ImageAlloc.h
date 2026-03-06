#pragma once
#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

class ImageAlloc
{
public:
    VmaAllocator allocator;
    VkImage handle;
    VmaAllocation allocation;
    VkImageView view;
    VkDevice device;

    ImageAlloc() = default;

    static ImageAlloc create(VmaAllocator allocator, 
        VkDevice device, 
        VkImageCreateInfo image_create_info, 
        VmaAllocationCreateFlags vma_flags, 
        VkImageAspectFlags aspect_mask);

    void destroy();
};  
