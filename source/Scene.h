#pragma once
#include<vector>
#include <memory>
#include "Model.h"

struct Entity
{
    Model* model;
    glm::mat4 transform;
};

struct Camera
{
    glm::vec3 pos;
    glm::mat4 view;
    glm::mat4 proj;
};

class Scene
{
public:
    std::vector<std::unique_ptr<Model>> models;
    std::vector<std::unique_ptr<Texture>> textures;
    std::vector<Entity> entities;
    Camera camera;
    glm::vec4 light_pos;

    void addEntity(Model* m, glm::vec3 pos)
    {
        Entity e;
        e.model = m;
        e.transform = glm::translate(glm::mat4(1.0f), pos);
        entities.push_back(e);
    }
};