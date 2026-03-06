#pragma once
#include <volk/volk.h>
#include "ImageAlloc.h"
class Texture
{
    ImageAlloc texture_image;
    VkSampler sampler;

};