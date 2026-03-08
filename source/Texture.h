#pragma once
#include <volk/volk.h>
#include "ImageAlloc.h"

class Texture
{
public:
    ImageAlloc image;
    VkSampler sampler;
    VkDescriptorImageInfo descriptor;
    uint32_t texture_index;

    void destroy(VkDevice device)
    {
        image.destroy();
    }

};