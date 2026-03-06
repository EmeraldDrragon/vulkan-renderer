#include "ImageAlloc.h"

ImageAlloc ImageAlloc::create(VmaAllocator allocator, 
    VkDevice device, 
    VkImageCreateInfo image_create_info, 
    VmaAllocationCreateFlags vma_flags, 
    VkImageAspectFlags aspect_mask)
{
    ImageAlloc img;
    img.allocator = allocator;
    img.device = device;
    VmaAllocationCreateInfo alloc_create_info = {
        .flags = vma_flags,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    vmaCreateImage(allocator, &image_create_info, &alloc_create_info, &img.handle, &img.allocation, nullptr);

    VkImageViewCreateInfo view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = img.handle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = image_create_info.format,
        .subresourceRange = {
            .aspectMask = aspect_mask,
            .baseMipLevel = 0,
            .levelCount = image_create_info.mipLevels,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vkCreateImageView(device, &view_create_info, nullptr, &img.view);
    return img;
}

void ImageAlloc::destroy()
{
    if(view != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, view, nullptr);
        view = VK_NULL_HANDLE;
    }
    if(handle != VK_NULL_HANDLE)
    {
        vmaDestroyImage(allocator, handle, allocation);
        handle = VK_NULL_HANDLE;
    }
}