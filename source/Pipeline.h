#pragma once
#include <volk/volk.h>
#include "Engine.h"
#include "Renderer.h"

class Pipeline
{
public:
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    Pipeline(Engine* engine, Renderer* renderer, Output* output)
    {
       
    }
};