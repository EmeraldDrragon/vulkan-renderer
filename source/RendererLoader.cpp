#include "RendererLoader.h"
#include "Scene.h"

void RendererLoader::setupShaderDataBuffers(Engine* engine)
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

void RendererLoader::setupSynchronizationObjects(Engine* engine, Output* output)
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
        
        engine->main_deletion_queue.push([=]()
        {
            vkDestroyFence(engine->device, fences[i], nullptr);
            vkDestroySemaphore(engine->device, present_semaphores[i], nullptr);
        });

    }
    render_semaphores.resize(output->swapchain_images.size());
    for(auto& semaphore : render_semaphores)
    {
        vkCreateSemaphore(engine->device, &semaphore_create_info, nullptr, &semaphore);
        engine->main_deletion_queue.push([=]()
        {
            vkDestroySemaphore(engine->device, semaphore, nullptr);
            
        });
    }

    std::cout << "synchronization objects created" << std::endl;
}

void RendererLoader::setupCommandBuffers(Engine* engine)
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
    engine->main_deletion_queue.push([=]()
    {
        vkDestroyCommandPool(engine->device, command_pool, nullptr);

    });
    std::cout << "command pool setup complete" << std::endl;
}

void RendererLoader::setupSamplers(Engine* engine)
{
    VkSamplerCreateInfo sampler_create_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 16.0f,
        .minLod = 0.0F,
        .maxLod = VK_LOD_CLAMP_NONE
    };
    vkCreateSampler(engine->device, &sampler_create_info, nullptr, &default_sampler);
    engine->main_deletion_queue.push([=]()
    {
        vkDestroySampler(engine->device, default_sampler, nullptr);
    });
}

void RendererLoader::loadModel(Engine* engine, Model* model)
{
    model->v_buf_size = sizeof(Vertex) * model->vertices.size();
    model->i_buf_size = sizeof(uint16_t) * model->indices.size();
    size_t size = model->v_buf_size + model->i_buf_size;
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    VmaAllocationCreateFlags vma_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    model->model_buffer = BufferAlloc::create(engine->allocator, engine->device, size, usage, vma_flags);
    memcpy(model->model_buffer.allocation_info.pMappedData, model->vertices.data(), model->v_buf_size);
    memcpy(((char*)model->model_buffer.allocation_info.pMappedData) + model->v_buf_size, model->indices.data(), model->i_buf_size);

    engine->main_deletion_queue.push([=]() mutable
    {
        model->model_buffer.destroy();
    });
    std::cout << "mesh uploaded to cpu" << std::endl;
}

void RendererLoader::setupDescriptors(Engine* engine, Scene* scene)
{
    VkDescriptorBindingFlags desc_var_flag = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
    VkDescriptorSetLayoutBindingFlagsCreateInfo desc_binding_flags = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
    .bindingCount = 1,
    .pBindingFlags = &desc_var_flag
    };
    VkDescriptorSetLayoutBinding desc_layout_binding_textures = {
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(scene->textures.size()),
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };
    VkDescriptorSetLayoutCreateInfo desc_layout_texture_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &desc_binding_flags,
        .bindingCount = 1,
        .pBindings = &desc_layout_binding_textures
    };
    vkCreateDescriptorSetLayout(engine->device, &desc_layout_texture_create_info, nullptr, &descriptor_set_layout_textures);

    VkDescriptorPoolSize pool_size = {
    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = static_cast<uint32_t>(scene->textures.size())
    };
    VkDescriptorPoolCreateInfo desc_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &pool_size
    };
    vkCreateDescriptorPool(engine->device, &desc_pool_create_info, nullptr, &descriptor_pool);

    uint32_t var_desc_count = static_cast<uint32_t>(scene->textures.size());
    VkDescriptorSetVariableDescriptorCountAllocateInfo var_desc_count_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
        .descriptorSetCount = 1,
        .pDescriptorCounts = &var_desc_count
    };
    VkDescriptorSetAllocateInfo texture_desc_set_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = &var_desc_count_alloc_info,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptor_set_layout_textures
    };
    vkAllocateDescriptorSets(engine->device, &texture_desc_set_alloc_info, &descriptor_set_textures);

    engine->main_deletion_queue.push([=]() 
    {
        vkDestroyDescriptorPool(engine->device, descriptor_pool, nullptr);
        vkDestroyDescriptorSetLayout(engine->device, descriptor_set_layout_textures, nullptr);
    });
}

void RendererLoader::updateSceneDescriptors(Engine* engine, Scene* scene)
{
    std::vector<VkDescriptorImageInfo> texture_descriptor_infos;
    for(const auto& tex : scene->textures)
    {
        texture_descriptor_infos.push_back(tex->descriptor);
    }

    VkWriteDescriptorSet write_desc_set = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set_textures,
        .dstBinding = 0,
        .descriptorCount = static_cast<uint32_t>(texture_descriptor_infos.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = texture_descriptor_infos.data()
    };
    vkUpdateDescriptorSets(engine->device, 1, &write_desc_set, 0, nullptr);
}

Texture* RendererLoader::loadTexture(Engine* engine, std::string filename)
{
    ktxTexture* ktx_texture = nullptr;
    KTX_error_code result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);
    if(result != KTX_SUCCESS || ktx_texture == nullptr)
    {
        std::cout << "could not load texture" << std::endl;
        return NULL;
    }

    Texture* tex = new Texture();

    VkImageCreateInfo texture_image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = ktxTexture_GetVkFormat(ktx_texture),
        .extent = {
            .width = ktx_texture->baseWidth,
            .height = ktx_texture->baseHeight,
            .depth = 1
        },
        .mipLevels = ktx_texture->numLevels,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    tex->image = ImageAlloc::create(engine->allocator, 
        engine->device, 
        texture_image_create_info, 
        0, 
        VK_IMAGE_ASPECT_COLOR_BIT);

    BufferAlloc staging = BufferAlloc::create(engine->allocator, 
        engine->device, 
        ktx_texture->dataSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);
    memcpy(staging.allocation_info.pMappedData, ktx_texture->pData, ktx_texture->dataSize);

    VkFence fence;
    VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    };
    vkCreateFence(engine->device, &fence_create_info, nullptr, &fence);
    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers(engine->device, &alloc_info, &cmd);
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(cmd, &begin_info);
    VkImageMemoryBarrier2 barrier_texture_image = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask = VK_ACCESS_2_NONE,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = tex->image.handle,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = ktx_texture->numLevels,
            .layerCount = 1
        }
    };
    VkDependencyInfo dep_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier_texture_image
    };
    vkCmdPipelineBarrier2(cmd, &dep_info);

    std::vector<VkBufferImageCopy> copy_regions;
    for(auto j = 0; j < ktx_texture->numLevels; j++)
    {
        ktx_size_t mipOffset;
        ktxTexture_GetImageOffset(ktx_texture, j, 0, 0, &mipOffset);
        copy_regions.push_back({
            .bufferOffset = mipOffset,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = (uint32_t)j,
                .layerCount = 1
            },
            .imageExtent = {
                .width = ktx_texture->baseWidth >> j,
                .height = ktx_texture->baseHeight >> j,
                .depth = 1
            },
        });
    }
    vkCmdCopyBufferToImage(cmd, staging.handle, tex->image.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copy_regions.size()), copy_regions.data());
    VkImageMemoryBarrier2 barrier_texture_read = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
        .image = tex->image.handle,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = ktx_texture->numLevels,
            .layerCount = 1
        }
    };
    dep_info.pImageMemoryBarriers = &barrier_texture_read;
    vkCmdPipelineBarrier2(cmd, &dep_info);
    vkEndCommandBuffer(cmd);
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd
    };
    vkQueueSubmit(engine->queue, 1, &submit_info, fence);
    vkWaitForFences(engine->device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(engine->device, fence, nullptr);
    vkFreeCommandBuffers(engine->device, command_pool, 1, &cmd);
    staging.destroy();

    tex->sampler = default_sampler;
    tex->descriptor = {
        .sampler = tex->sampler,
        .imageView = tex->image.view,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
    };

    ktxTexture_Destroy(ktx_texture);
    engine->main_deletion_queue.push([=]()
    {
        tex->destroy(engine->device);
        delete tex; 
    });

    return tex;
}

void RendererLoader::loadShaders(Engine* engine, const char* shader_file)
{
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
    slang_global_session->createSession(slang_session_desc, slang_session.writeRef());
    slang_module = slang_session->loadModuleFromSource("scene_shader", shader_file, nullptr, nullptr);
    slang_module->getTargetCode(0, spirv.writeRef());
    VkShaderModuleCreateInfo shader_module_create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv->getBufferSize(),
        .pCode = (uint32_t*)spirv->getBufferPointer()
    };
    vkCreateShaderModule(engine->device, &shader_module_create_info, nullptr, &shader_module);    

    engine->main_deletion_queue.push([=]()
    {
        vkDestroyShaderModule(engine->device, shader_module, nullptr);
    });
}