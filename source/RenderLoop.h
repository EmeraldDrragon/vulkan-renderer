#pragma once
#include <volk/volk.h>
#include "Engine.h"
#include "Output.h"
#include "Pipeline.h"
#include "RendererLoader.h"
#include "Scene.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

class RenderLoop
{
    uint32_t frame_index = 0;
    uint32_t image_index = 0;
    bool quit = false;
    bool update_swapchain = false;
    uint32_t selected_instance = 0;
    uint64_t last_time = 0;

public:
    //call in the main after all setup is done
    void render(Engine* engine, Output* output, RendererLoader* loader, Pipeline* pipeline, Scene* scene);
};
