#pragma once

#include <volk/volk.h>
#include "BufferAlloc.h"
#include <tiny_obj_loader.h>
#include <ktx.h>
#include <ktxvulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Texture.h"

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};
class Model
{
public:
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    BufferAlloc model_buffer;
    VkDeviceSize v_buf_size;
    VkDeviceSize i_buf_size;
    Texture* texture;

    Model(std::string path)
    {
        std::cout << "loading model" << std::endl;
        std::ifstream filename(path);
        tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, &filename);
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
        std::cout << "loading model complete" << std::endl;
    }
};