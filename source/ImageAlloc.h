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
        VkImageAspectFlags aspect_mask)
    {
        ImageAlloc img;
        img.allocator = allocator;
        VmaAllocationCreateInfo alloc_create_info = {
            .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        vmaCreateImage(allocator, &image_create_info, &alloc_create_info, &img.handle, &img.allocation, nullptr);

    }

};  