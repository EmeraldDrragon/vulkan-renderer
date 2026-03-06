#pragma once

#include <volk/volk.h>
#include "Engine.h"
#include "Output.h"
#include "ImageAlloc.h"
#include "BufferAlloc.h"
#include <tiny_obj_loader.h>
#include <ktx.h>
#include <ktxvulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};
class Model
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    BufferAlloc model_buffer;
    VkDeviceSize index_count;
    VkDeviceSize v_buf_size;
    VkDeviceSize i_buf_size;
    //add dedicated texture here I think


    Model(std::istream* filename)
    {
        tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, filename);
        for(auto& index : shapes[0].mesh.indices)
        {
            Vertex v = {
                .pos = {
                    attrib.vertices[index.vertex_index * 3],
                    -attrib.vertices[index.vertex_index * 3 + 1],
                    attrib.vertices[index.vertex_index * 3 + 2]
                },
                .normal = {
                    attrib.normals[index.normal_index * 3],
                    -attrib.normals[index.normal_index * 3 + 1],
                    attrib.normals[index.normal_index * 3 + 2]
                },
                .uv = {
                    attrib.texcoords[index.texcoord_index * 2],
                    1.0 - attrib.texcoords[index.texcoord_index * 2 + 1]
                }
            };
            vertices.push_back(v);
            indices.push_back(indices.size());
        }
    }

    // call this method from inside scene, maybe actually from renderer 
    // and put this method into renderer with texture loading
    void loadMeshToGpu(Engine* engine)
    {
        v_buf_size = sizeof(Vertex) * vertices.size();
        i_buf_size = sizeof(uint16_t) * indices.size();
        size_t size = v_buf_size + i_buf_size;
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        VmaAllocationCreateFlags vma_flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        model_buffer = BufferAlloc::create(engine->allocator, engine->device, size, usage, vma_flags);
        memcpy(model_buffer.allocation_info.pMappedData, vertices.data(), v_buf_size);
        memcpy(((char*)model_buffer.allocation_info.pMappedData)+v_buf_size, indices.data(), i_buf_size);

        engine->main_deletion_queue.push([=]() mutable
        {
            model_buffer.destroy();
        });
        std::cout << "mesh uploaded to cpu" << std::endl;
    }

};