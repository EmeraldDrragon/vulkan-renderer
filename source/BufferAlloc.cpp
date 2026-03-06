#include "BufferAlloc.h"

BufferAlloc BufferAlloc::create(VmaAllocator allocator, 
    VkDevice device, 
    size_t size, 
    VkBufferUsageFlags usage, 
    VmaAllocationCreateFlags vma_flags)
{
    BufferAlloc buf;
    buf.allocator = allocator;
    VkBufferCreateInfo buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage
    };
    VmaAllocationCreateInfo vma_alloc_info = {
        .flags = vma_flags,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    vmaCreateBuffer(allocator, &buffer_create_info, &vma_alloc_info, &buf.handle, &buf.allocation, &buf.allocation_info);
    if(usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        VkBufferDeviceAddressInfo device_address_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = buf.handle
        };
        buf.device_address = vkGetBufferDeviceAddress(device, &device_address_info);
    }
    return buf;
}

void BufferAlloc::destroy()
{
    if(handle != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(allocator, handle, allocation);
        handle = VK_NULL_HANDLE;
    }
}