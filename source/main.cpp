#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#define VMA_IMPLEMENTATION
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
// #define TINYOBJLOADER_IMPLEMENTATION

#include <iostream>

#include "Engine.h"
#include "Output.h"
#include "RendererLoader.h"
#include "Scene.h"
#include "Model.h"
#include "Texture.h"
#include "Pipeline.h"
#include "RenderLoop.h"

//main.cpp Usage
//1. Create Engine
//2. Create Output
//3. Create Renderer loader
//4. Create Scene
//5. Create models with a file
//6. Load them into gpu thorugh renderer
//7. Load textures into gpu through renderer with a file
//8. Assign textures to models
//9. Put model into scene
//10 Put texture into scene
//11. Add entities to scene
//12. update descriptors with a renderer
//13. Create pipeline
//14. render loop
//15. cleanup

int main()
{
    Engine engine;
    Output output(&engine);
    RendererLoader loader(&engine, &output);
    Scene scene;

    auto cat = std::make_unique<Model>("assets/Cat.obj");
    loader.loadModel(&engine, cat.get());

    Texture* cat_texture = loader.loadTexture(&engine, "assets/cat0.ktx");
    cat->texture = cat_texture;

    scene.models.push_back(std::move(cat));
    scene.textures.push_back(std::unique_ptr<Texture>(cat_texture));
    scene.addEntity(scene.models.back().get(), glm::vec3(0.0f, 0.0f, 0.0f));

    scene.light_pos = glm::vec4(0.0f, -10.0f, 10.0f, 0.0f);

    scene.camera.pos = glm::vec3(0.0f, 0.0f, -0.0f);
    scene.camera.view = glm::translate(glm::mat4(1.0f), scene.camera.pos);
    float aspect = (float)output.window_width / (float)output.window_height;
    scene.camera.proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 32.0f);

    loader.setupDescriptors(&engine, &scene);
    loader.updateSceneDescriptors(&engine, &scene);

    loader.loadShaders(&engine, "assets/shader.slang");

    Pipeline pipeline(&engine, &loader, &output);

    RenderLoop loop;
    loop.render(&engine, &output, &loader, &pipeline, &scene);

    engine.cleanup();
    
    return 0;
}