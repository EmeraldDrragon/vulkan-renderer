#pragma once
#include <volk/volk.h>
#include "Engine.h"
#include "RendererLoader.h"

struct PushConstants 
{
    glm::mat4 model_mat;
    VkDeviceAddress scene;
    uint32_t texture_index;
    uint32_t instance_id;
};
class Pipeline
{
public:
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    Pipeline(Engine* engine, RendererLoader* loader, Output* output);
};