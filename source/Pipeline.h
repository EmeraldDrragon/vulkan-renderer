#pragma once
#include <volk/volk.h>
#include "Engine.h"
#include "RendererLoader.h"

class Pipeline
{
public:
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    Pipeline(Engine* engine, RendererLoader* loader, Output* output)
    {
       
    }
};