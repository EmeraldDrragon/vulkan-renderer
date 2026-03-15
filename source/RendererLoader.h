#pragma once

#include <volk/volk.h>

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <fstream>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vma/vk_mem_alloc.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "slang/slang.h"
#include "slang/slang-com-ptr.h"

#include <ktx.h>
#include <ktxvulkan.h>

#include "BufferAlloc.h"
#include "Engine.h"
#include "Output.h"
#include "ImageAlloc.h"
#include "Texture.h"
#include "Model.h"

class Scene;

struct SceneData
{
    glm::mat4 projection;
    glm::mat4 view;
    glm::vec4 light_pos;
    uint32_t selected_instance;
    float pad[3];
};


constexpr uint32_t max_frames_in_flight = 2;


class RendererLoader
{
public:
    std::array<BufferAlloc, max_frames_in_flight> shader_data_buffers;

    std::array<VkFence, max_frames_in_flight> fences;
    std::array<VkSemaphore, max_frames_in_flight> present_semaphores;
    std::vector<VkSemaphore> render_semaphores;

    std::array<VkCommandBuffer, max_frames_in_flight> command_buffers;

    std::array<VkDeviceAddress, max_frames_in_flight> shader_data_addresses;


    VkCommandPool command_pool;

    VkSampler default_sampler;

    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout descriptor_set_layout_textures;
    VkDescriptorSet descriptor_set_textures;

    Slang::ComPtr<slang::IGlobalSession> slang_global_session;
    Slang::ComPtr<slang::ISession> slang_session;
    Slang::ComPtr<slang::IModule> slang_module;
    Slang::ComPtr<ISlangBlob> spirv;
    VkShaderModule shader_module;



    RendererLoader(Engine* engine, Output* output)
    {
        setupShaderDataBuffers(engine);
        setupSynchronizationObjects(engine, output);
        setupCommandBuffers(engine);
        setupSamplers(engine);
    }

    void setupShaderDataBuffers(Engine* engine);

    void setupSynchronizationObjects(Engine* engine, Output* output);

    void setupCommandBuffers(Engine* engine);

    void setupSamplers(Engine* engine);

    //call from main to load models
    void loadModel(Engine* engine, Model* model);

    //call from main to setup descriptors after entities loaded
    void setupDescriptors(Engine* engine, Scene* scene);

    //call from main to update descriptors after entities loaded
    void updateSceneDescriptors(Engine* engine, Scene* scene);

    //call from main to load textures
    Texture* loadTexture(Engine* engine, std::string filename);

    //call from main to load shader file
    void loadShaders(Engine* engine, const char* shader_file);
};