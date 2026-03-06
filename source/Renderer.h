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

#include <tiny_obj_loader.h>

#include "BufferAlloc.h"
#include "Engine.h"
#include "Output.h"

struct shaderData
{
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model[3];
    glm::vec4 light_pos = {0.0f, -10.0f, 10.0f, 0.0f};
    uint32_t selected = 1;
};


constexpr uint32_t max_frames_in_flight = 2;


class Renderer
{
public:
    std::array<BufferAlloc, max_frames_in_flight> shader_data_buffers;

    std::array<VkFence, max_frames_in_flight> fences;
    std::array<VkSemaphore, max_frames_in_flight> present_semaphores;
    std::vector<VkSemaphore> render_semaphores;

    std::array<VkCommandBuffer, max_frames_in_flight> command_buffers;
    VkCommandPool command_pool;

    Slang::ComPtr<slang::IGlobalSession> slang_global_session;


    Renderer(Engine* engine, Output* output)
    {
        setupShaderDataBuffers(engine);
        setupSynchronizationObjects(engine, output);
        setupCommandBuffers(engine);
    }

    void setupShaderDataBuffers(Engine* engine)
    {
        //cosntructor sets up shared resources and synchronization objects
        for(auto i = 0; i < max_frames_in_flight; i++)
        {
            size_t size = sizeof(shaderData);
            VkBufferUsageFlags usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            VmaAllocationCreateFlags vma_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            shader_data_buffers[i] = BufferAlloc::create(engine->allocator, engine->device, size, usage, vma_flags);
            engine->main_deletion_queue.push([=]() mutable
            {
                shader_data_buffers[i].destroy();
            });
        }
        std::cout << "shader data buffers setup complete" << std::endl;
    }

    void setupSynchronizationObjects(Engine* engine, Output* output)
    {
        VkSemaphoreCreateInfo semaphore_create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };
        VkFenceCreateInfo fence_create_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };
        for(auto i = 0; i < max_frames_in_flight; i++)
        {
            vkCreateFence(engine->device, &fence_create_info, nullptr, &fences[i]);
            vkCreateSemaphore(engine->device, &semaphore_create_info, nullptr, &present_semaphores[i]);
            //TODO add to deletion queue here
        }
        render_semaphores.resize(output->swapchain_images.size());
        for(auto& semaphore : render_semaphores)
        {
            vkCreateSemaphore(engine->device, &semaphore_create_info, nullptr, &semaphore);
            //TODO add to deletion queue here
        }

        std::cout << "synchronization objects created" << std::endl;
    }

    void setupCommandBuffers(Engine* engine)
    {
        VkCommandPoolCreateInfo command_pool_create_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = engine->queue_family_index
        };
        vkCreateCommandPool(engine->device, &command_pool_create_info, nullptr, &command_pool);
        VkCommandBufferAllocateInfo command_buffer_alloc_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = command_pool,
            .commandBufferCount = max_frames_in_flight
        };
        vkAllocateCommandBuffers(engine->device, &command_buffer_alloc_info, command_buffers.data());
        //TODO add to deletion queue here
        std::cout << "command pool setup complete" << std::endl;
    }

    //call from main to load shader file
    void loadShaders(std::string shader_file)
    {
        Slang::ComPtr<slang::IGlobalSession> slang_global_session;
        slang::createGlobalSession(slang_global_session.writeRef());
        auto slang_targets = std::to_array<slang::TargetDesc>({{
            .format = SLANG_SPIRV,
            .profile = slang_global_session->findProfile("spirv_1_4")
        }});
        auto slang_options = std::to_array<slang::CompilerOptionEntry>({{slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1}}});
        slang::SessionDesc slang_session_desc = {
            .targets = slang_targets.data(),
            .targetCount =  SlangInt(slang_targets.size()),
            .defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
            .compilerOptionEntries = slang_options.data(),
            .compilerOptionEntryCount = uint32_t(slang_options.size())
        };
        Slang::ComPtr<slang::ISession> slang_session;
        slang_global_session->createSession(slang_session_desc, slang_session.writeRef());
        Slang::ComPtr<slang::IModule> slang_module{
            slang_session->loadModuleFromSource("triangle", shader_file, nullptr, nullptr)
        };
        Slang::ComPtr<ISlangBlob> spirv;
        slang_module->getTargetCode(0, spirv.writeRef());
        VkShaderModuleCreateInfo shader_module_create_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = spirv->getBufferSize(),
            .pCode = (uint32_t*)spirv->getBufferPointer()
        };
        VkShaderModule shader_module;
        vkCreateShaderModule(device, &shader_module_create_info, nullptr, &shader_module);

        std::cout << "shader loaded" << std::endl;
    }
};