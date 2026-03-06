#pragma once
#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

class BufferAlloc
{
public:
    VmaAllocator allocator;
    VkBuffer handle;
    VmaAllocation allocation;
    VmaAllocationInfo allocation_info;
    VkDeviceAddress device_address;

    BufferAlloc() = default;

    static BufferAlloc create(VmaAllocator allocator, 
        VkDevice device, 
        size_t size, 
        VkBufferUsageFlags usage, 
        VmaAllocationCreateFlags vma_flags);
        
    void destroy();
};