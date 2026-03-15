#include "RenderLoop.h"

void RenderLoop::render(Engine* engine, Output* output, RendererLoader* loader, Pipeline* pipeline, Scene* scene)
{
    std::cout << "starting render loop" << std::endl;
    last_time = SDL_GetTicks();

    while(!quit)
    {
        vkWaitForFences(engine->device, 1, &loader->fences[frame_index], VK_TRUE, UINT64_MAX);
        vkResetFences(engine->device, 1, &loader->fences[frame_index]);


        vkAcquireNextImageKHR(engine->device, output->swapchain, UINT64_MAX, loader->present_semaphores[frame_index], VK_NULL_HANDLE, &image_index);


        scene->camera.proj = glm::perspective(glm::radians(45.0f), (float)output->window_width / (float)output->window_height, 0.1f, 32.0f);
        scene->camera.view = glm::translate(glm::mat4(1.0f), scene->camera.pos);


        SceneData scene_data = {
            .projection = scene->camera.proj,
            .view = scene->camera.view,
            .light_pos = scene->light_pos,
            .selected_instance = selected_instance
        };
        SceneData* mapped = reinterpret_cast<SceneData*>(loader->shader_data_buffers[frame_index].allocation_info.pMappedData);
        *mapped = scene_data;


        VkCommandBuffer cmd = loader->command_buffers[frame_index];
        VkCommandBufferBeginInfo begin = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        vkBeginCommandBuffer(cmd, &begin);


        std::array<VkImageMemoryBarrier2, 2> barriers = {
            VkImageMemoryBarrier2{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .image = output->swapchain_images[image_index],
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,
                    .layerCount = 1
                }
            },
            VkImageMemoryBarrier2{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .image = output->depth_attachment.handle,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                    .levelCount = 1,
                    .layerCount = 1
                }
            }
        };
        VkDependencyInfo barrier_dependency_info = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 2,
            .pImageMemoryBarriers = barriers.data()
        };
        vkCmdPipelineBarrier2(cmd, &barrier_dependency_info);


        VkRenderingAttachmentInfo color_attachment_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = output->swapchain_image_views[image_index],
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue{
                .color{ 0.0f, 0.0f, 0.0f, 1.0f }
            }
        };
        VkRenderingAttachmentInfo depth_attachment_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = output->depth_attachment.view,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue = {
                .depthStencil = {1.0f,  0}
            }
        };
        VkRenderingInfo rendering_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {
                .extent = {
                    .width = static_cast<uint32_t>(output->window_width),
                    .height = static_cast<uint32_t>(output->window_height)
                }
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment_info,
            .pDepthAttachment = &depth_attachment_info
        };
        vkCmdBeginRendering(cmd, &rendering_info);


        VkViewport viewport = {
            .width = static_cast<float>(output->window_width),
            .height = static_cast<float>(output->window_height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        VkRect2D scissor = {
            .extent = {
                .width = static_cast<uint32_t>(output->window_width),
                .height = static_cast<uint32_t>(output->window_height)
            }
        };
        vkCmdSetScissor(cmd, 0, 1, &scissor);


        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline_layout, 0, 1, &loader->descriptor_set_textures, 0, nullptr);


        for(size_t i = 0; i < scene->entities.size(); ++i)
        {
            const Entity& e = scene->entities[i];
            if(!e.model || !e.model->texture)
            {
                continue;
            }
            PushConstants pc = {
                .model_mat = e.transform,
                .scene = loader->shader_data_addresses[frame_index],
                .texture_index = e.model->texture->texture_index,
                .instance_id = static_cast<uint32_t>(i)
            };
            vkCmdPushConstants(cmd, pipeline->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pc);
            VkDeviceSize vOffset = 0;
            VkDeviceSize iOffset = e.model->v_buf_size;
            vkCmdBindVertexBuffers(cmd, 0, 1, &e.model->model_buffer.handle, &vOffset);
            vkCmdBindIndexBuffer(cmd, e.model->model_buffer.handle, iOffset, VK_INDEX_TYPE_UINT16);
            vkCmdDrawIndexed(cmd, static_cast<uint32_t>(e.model->indices.size()), 1, 0, 0, 0);
        }
        vkCmdEndRendering(cmd);
        VkImageMemoryBarrier2 barrier_present = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .image = output->swapchain_images[image_index],
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1 
            }
        };
        VkDependencyInfo barrier_present_dependency_info = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier_present
        };
        vkCmdPipelineBarrier2(cmd, &barrier_present_dependency_info);
        vkEndCommandBuffer(cmd);


        VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &loader->present_semaphores[frame_index],
            .pWaitDstStageMask = &wait_stages,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &loader->render_semaphores[image_index],
        };
        vkQueueSubmit(engine->queue, 1, &submit_info, loader->fences[frame_index]);


        frame_index = (frame_index + 1) % max_frames_in_flight;


        VkPresentInfoKHR present_info{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &loader->render_semaphores[image_index],
            .swapchainCount = 1,
            .pSwapchains = &output->swapchain,
            .pImageIndices = &image_index
        };
        vkQueuePresentKHR(engine->queue, &present_info);


        float elapsed_time{ (SDL_GetTicks() - last_time) / 1000.0f };
        last_time = SDL_GetTicks();
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_EVENT_QUIT)
            {
                quit = true;
                break;
            }
            if(event.type == SDL_EVENT_MOUSE_MOTION && (event.motion.state & SDL_BUTTON_LMASK))
            {
                if (selected_instance < scene->entities.size())
                {
                    float sens = elapsed_time * 2.0f;
                    glm::quat rotY = glm::angleAxis( event.motion.xrel * sens, glm::vec3(0,1,0));
                    glm::quat rotX = glm::angleAxis(-event.motion.yrel * sens, glm::vec3(1,0,0));
                    scene->entities[selected_instance].transform = scene->entities[selected_instance].transform * glm::mat4_cast(rotY * rotX);
                }
            }
            if(event.type == SDL_EVENT_MOUSE_WHEEL)
            {
                scene->camera.pos.z += event.wheel.y * elapsed_time * 10.0f;
            }

            if(event.type == SDL_EVENT_KEY_DOWN)
            {
                if(event.key.key == SDLK_PLUS || event.key.key == SDLK_KP_PLUS)
                {
                    selected_instance = (selected_instance + 1) % static_cast<uint32_t>(scene->entities.size());
                }
                if(event.key.key == SDLK_MINUS || event.key.key == SDLK_KP_MINUS)
                {
                    selected_instance = selected_instance == 0 ? static_cast<uint32_t>(scene->entities.size())-1 : selected_instance-1;
                }
            }
            if(event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                update_swapchain = true;
            }
        }
        if(update_swapchain)
        {
            update_swapchain = false;
            vkDeviceWaitIdle(engine->device);

            SDL_GetWindowSize(engine->window, &output->window_width, &output->window_height);

            for (auto& v : output->swapchain_image_views)
            {
                vkDestroyImageView(engine->device, v, nullptr);
            }
            output->swapchain_image_views.clear();

            VkSwapchainKHR old = output->swapchain;

            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(engine->physical_device, engine->surface, &engine->surface_caps);

            output->getImageFormat(engine);
            VkSwapchainCreateInfoKHR swapchain_create_info = {
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .surface = engine->surface,
                .minImageCount = engine->surface_caps.minImageCount + 1,
                .imageFormat = output->image_format,
                .imageColorSpace = output->image_color_space,
                .imageExtent = {
                    .width = static_cast<uint32_t>(output->window_width),
                    .height = static_cast<uint32_t>(output->window_height)
                },
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .preTransform = engine->surface_caps.currentTransform,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode = VK_PRESENT_MODE_FIFO_KHR,
                .oldSwapchain = old
            };
            vkCreateSwapchainKHR(engine->device, &swapchain_create_info, nullptr, &output->swapchain);

            output->createImageAndImageView(engine);

            if(old != VK_NULL_HANDLE)
            {
                vkDestroySwapchainKHR(engine->device, old, nullptr);
            }

            output->depth_attachment.destroy();
            output->createDepthImageAndImageView(engine);
        }
    }
        std::cout << "render loop finished" << std::endl;

}