// STL include directives
#include <iostream>
#include <vector>

//GLEW & OGL include directives
#ifndef __APPLE__
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#define __gl_h_ /* Prevent inclusion of the old gl.h */
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#endif

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#else
// GLFW is necessary to handle the OpenGL context
#include <GLFW/glfw3.h>
#endif

// OpenGL Mathematics Library
#include <glm/glm.hpp> // glm::vec3
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp>

// Timer
#include <chrono>

// STB image loader
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Custom Classes
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "Scene_Objects.h"
#include "Light.h"

// STATE
enum RenderState {
    phong
};
GLuint RENDER_STATE = phong;
bool   OBJECT_SELECTED = false;
GLuint ACTIVE_OBJECT = 0;
GLuint SCREEN_HEIGHT = 800;
GLuint SCREEN_WIDTH = 1280;
GLfloat SCREEN_GAMMA = 2.2f;
bool   LIGHT_MOTION = false;
bool   WITH_BLINN = false;
bool   HIGHLIGHT_SHADOWS = false;
bool   TEST_RUNNING = false;
const unsigned int SHADOW_WIDTH = 1920;
const unsigned int SHADOW_HEIGHT = 1920;
const float SHADOW_ASPECT = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;

Camera camera(static_cast<float>(SCREEN_WIDTH)/static_cast<float>(SCREEN_HEIGHT),
              SCREEN_WIDTH,
              SCREEN_HEIGHT);
glm::vec3 LIGHT_START = glm::vec3(0.0f, 10.0f, 1.0f);
Light sceneLight (LIGHT_START,
                  glm::vec3(0.05f),
                  glm::vec3(0.5f),
                  glm::vec3(0.7f));
auto t_start = std::chrono::high_resolution_clock::now();
GLfloat LIGHT_SPEED = 2.0f;
GLuint currSky = 0;
Material material (glm::vec3(0.1f,0.1f,0.1f),
                   glm::vec3 (0.3f,0.1f, 0.7f),
                   glm::vec3(1.0f),
                   300.0f, true);

// CONSTANTS
glm::vec3 COLOR_GRAY = glm::vec3(0.5f);
glm::vec3 COLOR_CYAN = glm::vec3(0.66,0.9f,0.96f);
glm::vec3 COLOR_BLUE = glm::vec3(0.11f, 0.45f, 0.96f);
glm::vec3 COLOR_PURPLE = glm::vec3(0.3f, 0.1f, 0.7f);
glm::vec3 COLOR_MAGENTA= glm::vec3(0.86f,0.14f,0.82f);
glm::vec3 COLOR_RED = glm::vec3(0.86f, 0.2f, 0.14f);
glm::vec3 COLOR_ORANGE = glm::vec3(0.86f, 0.43f, 0.2f);
glm::vec3 COLOR_YELLOW = glm::vec3(0.3f,0.75f,0.0f);
glm::vec3 COLOR_GREEN = glm::vec3(0.6f, 0.81f, 0.7f);

// CONTAINERS
std::vector<Scene_Object> scene;
std::vector<Mesh> meshes;
std::vector<GLuint> skies;
std::vector<glm::vec3> rainbow;

// Functions
int main();
void updateTimeBased();
void testScene();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
glm::vec3 createRay(glm::vec2 screenCoords);
GLuint loadTexture(const char* path);
GLuint loadCubemap(std::vector<std::string> faces, bool flip);

// Shaders
const char* vertexPhongSource = "#version 330 core\n"
                                "layout (location = 0) in vec3 pos;\n"
                                "layout (location = 2) in vec3 normal;\n"
                                "\n"
                                "uniform mat4 model;\n"
                                "uniform mat4 proj;\n"
                                "uniform mat4 view;\n"
                                "uniform mat4 lightSpaceMatrix;\n"
                                "\n"
                                "out vec3 FragPos;\n"
                                "out vec3 Normal;\n"
                                "out vec4 FragPosLightSpace;\n"
                                "\n"
                                "void main(){\n"
                                "    Normal = mat3(transpose(inverse(model))) * normal;\n"
                                "    FragPos = vec3(model * vec4(pos, 1.0));\n"
                                "    gl_Position = proj * view * model * vec4(pos,1.0);\n"
                                "    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);\n"
                                "}";

const char* fragmentPhongSource = "#version 330 core\n"
                                  "\n"
                                  "in vec3 FragPos;\n"
                                  "in vec3 Normal;\n"
                                  "in vec4 FragPosLightSpace;\n"
                                  "\n"
                                  "struct Material {\n"
                                  "    vec3 ambient;\n"
                                  "    vec3 diffuse;\n"
                                  "    vec3 specular;\n"
                                  "    float shininess;\n"
                                  "};\n"
                                  "\n"
                                  "struct Light {\n"
                                  "    vec3 position;\n"
                                  "    vec3 ambient;\n"
                                  "    vec3 diffuse;\n"
                                  "    vec3 specular;\n"
                                  "};\n"
                                  "\n"
                                  "uniform vec3 viewPos;\n"
                                  "uniform Material material;\n"
                                  "uniform Light light;\n"
                                  "uniform bool withBlinn;\n"
                                  "uniform float screenGamma;\n"
                                  "uniform bool highlightShadows;\n"
                                  "\n"
                                  "uniform sampler2D shadowMap;\n"
                                  "\n"
                                  "out vec4 FragColor;\n"
                                  "\n"
                                  "float ShadowCalculation(vec4 fragPosLightSpace){\n"
                                  "    // shadow algorithms sourced from learnopengl.com\n"
                                  "    // De-homogenize the coordinates\n"
                                  "    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;\n"
                                  "\n"
                                  "    // Normalize\n"
                                  "    projCoords = projCoords * 0.5 + 0.5;\n"
                                  "\n"
                                  "    // get closest depth value from light's perspective\n"
                                  "    float closestDepth = texture(shadowMap, projCoords.xy).r;\n"
                                  "    float currentDepth = projCoords.z;\n"
                                  "\n"
                                  "    // calculate bias to eliminate shadow acne\n"
                                  "    vec3 normal = normalize(Normal);\n"
                                  "    vec3 lightDir = normalize(light.position - FragPos);\n"
                                  "    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.005);\n"
                                  "\n"
                                  "    //convolve the shadow depth with a box kernel for smoothing\n"
                                  "    float shadow = 0.0;\n"
                                  "    vec2 texelSize = vec2(1.0) / textureSize(shadowMap, 0);\n"
                                  "    for(int x = -1; x <= 1; ++x){\n"
                                  "        for(int y = -1; y<= 1; ++y){\n"
                                  "           float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x,y) * texelSize).r;\n"
                                  "           shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;\n"
                                  "        }\n"
                                  "    }\n"
                                  "    shadow /= 9.0;\n"
                                  "\n"
                                  "    if(projCoords.z > 1.0)\n"
                                  "        shadow = 0.0;\n"
                                  "    return shadow;\n"
                                  "}\n"
                                  "\n"
                                  "void main(){\n"
                                  "    // general variables\n"
                                  "    vec3 ambient, diffuse, specular;\n"
                                  "    float spec;\n"
                                  "    vec3 normal = normalize(Normal);\n"
                                  "    vec3 lightDir = normalize(light.position - FragPos);\n"
                                  "    vec3 viewDir = normalize(viewPos - FragPos);\n"
                                  "    vec3 reflectDir = reflect(-lightDir, normal);\n"
                                  "    vec3 halfwayDir = normalize(lightDir + viewDir);\n"
                                  "    float lambertian = max(dot(lightDir, normal), 0.0);\n"
                                  "\n"
                                  "    // ambient light value\n"
                                  "    ambient = light.ambient * material.ambient;\n"
                                  "\n"
                                  "    // calculate shadows\n"
                                  "    float shadow = ShadowCalculation(FragPosLightSpace);\n"
                                  "\n"
                                  "\n"
                                  "    if (lambertian > 0.0){\n"
                                  "        if(highlightShadows && shadow > 0.0){\n"
                                  "            ambient = vec3(1.0, 0.0, 0.0);\n"
                                  "            diffuse = vec3(1.0, 0.0, 0.0);\n"
                                  "            specular = vec3(1.0,0.0,0.0);\n"
                                  "        }\n"
                                  "        else{\n"
                                  "            // diffuse light calculation\n"
                                  "            diffuse = ((1.0 - shadow) * light.diffuse) * (lambertian * material.diffuse);\n"
                                  "\n"
                                  "            // specular light calculation\n"
                                  "            if(withBlinn){\n"
                                  "            spec = pow(max(dot(normal, halfwayDir), 0.0), 4.0 * material.shininess);\n"
                                  "            }\n"
                                  "            else{\n"
                                  "                spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);\n"
                                  "            }\n"
                                  "            specular = ((1.0 - shadow) * light.specular) * (spec * material.specular);\n"
                                  "        }\n"
                                  "    }\n"
                                  "    else{\n"
                                  "        if(highlightShadows){\n"
                                  "            ambient = vec3(1.0, 0.0, 0.0);\n"
                                  "            diffuse = vec3(1.0, 0.0, 0.0);\n"
                                  "            specular = vec3(1.0,0.0,0.0);\n"
                                  "        }\n"
                                  "        else{\n"
                                  "         specular = vec3(0.0);\n"
                                  "         diffuse = vec3(0.0);\n"
                                  "        }\n"
                                  "    }\n"
                                  "\n"
                                  "    // Gamma Correction\n"
                                  "    vec3 colorLinear = ambient + diffuse + specular;\n"
                                  "    vec3 colorGammaCorrected = pow(colorLinear, vec3(1.0 / screenGamma));\n"
                                  "    // Ouput Color\n"
                                  "    FragColor = vec4(colorGammaCorrected, 1.0);\n"
                                  "}";

const char* vertexReflectionSource = "#version 330 core\n"
                                     "layout (location = 0) in vec3 pos;\n"
                                     "layout (location = 2) in vec3 normal;\n"
                                     "\n"
                                     "uniform mat4 model;\n"
                                     "uniform mat4 view;\n"
                                     "uniform mat4 proj;\n"
                                     "\n"
                                     "out vec3 Normal;\n"
                                     "out vec3 FragPos;\n"
                                     "\n"
                                     "void main(){\n"
                                     "    Normal = mat3(transpose(inverse(model))) * normal;\n"
                                     "    FragPos = vec3(model * vec4(pos, 1.0));\n"
                                     "    gl_Position = proj * view * model * vec4(pos, 1.0);\n"
                                     "}";

const char* fragmentReflectionSource = "#version 330 core\n"
                                       "\n"
                                       "in vec3 Normal;\n"
                                       "in vec3 FragPos;\n"
                                       "\n"
                                       "struct Light {\n"
                                       "    vec3 position;\n"
                                       "    vec3 ambient;\n"
                                       "    vec3 diffuse;\n"
                                       "    vec3 specular;\n"
                                       "};\n"
                                       "\n"
                                       "uniform vec3 viewPos;\n"
                                       "uniform samplerCube skybox;\n"
                                       "uniform Light light;\n"
                                       "uniform float screenGamma;\n"
                                       "\n"
                                       "out vec4 FragColor;\n"
                                       "\n"
                                       "void main(){\n"
                                       "    // general variables\n"
                                       "    vec3 ambient, diffuse, specular;\n"
                                       "    float spec;\n"
                                       "    vec3 normal = normalize(Normal);\n"
                                       "    vec3 lightDir = normalize(light.position - FragPos);\n"
                                       "    vec3 viewDir = normalize(viewPos - FragPos);\n"
                                       "    vec3 reflectDir = reflect(-lightDir, normal);\n"
                                       "    vec3 halfwayDir = normalize(lightDir + viewDir);\n"
                                       "    float lambertian = max(dot(lightDir, normal), 0.0);\n"
                                       "\n"
                                       "    // Calculate color due to texture\n"
                                       "    vec3 incidence = normalize(FragPos - viewPos);\n"
                                       "    vec3 reflection = reflect(incidence, normalize(Normal));\n"
                                       "    vec4 fragTex = vec4(texture(skybox, reflection).rgb, 1.0);\n"
                                       "\n"
                                       "    // turn ambient off\n"
                                       "    // ambient = light.ambient * vec3(fragTex);\n"
                                       "\n"
                                       "    // diffuse is simply the color here\n"
                                       "    diffuse = vec3(fragTex);\n"
                                       "\n"
                                       "    // specular intensity to add shine spots\n"
                                       "    spec = pow(max(dot(normal, halfwayDir), 0.0), 1200.0);\n"
                                       "    specular = light.specular * (spec * vec3(fragTex));\n"
                                       "\n"
                                       "    // Gamma Correction\n"
                                       "    vec3 colorLinear = ambient + diffuse + specular;\n"
                                       "    vec3 colorGammaCorrected = pow(colorLinear, vec3(1.0 / screenGamma));\n"
                                       "    FragColor = vec4(colorGammaCorrected, 1.0);\n"
                                       "}";

const char* vertexSkyboxSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 pos;"
                                 "uniform mat4 projMat;"
                                 "uniform mat4 viewMat;"
                                 "out vec3 TexCoords;"
                                 "void main(){"
                                 "TexCoords = pos;"
                                 "vec4 viewPos = projMat * viewMat * vec4(pos,1.0);"
                                 "gl_Position = viewPos.xyww;"
                                 "}\0";

const char* fragmentSkyboxSource = "#version 330 core\n"
                                   "in vec3 TexCoords;"
                                   "uniform samplerCube skybox;"
                                   "out vec4 FragColor;"
                                   "void main(){"
                                   "FragColor = texture(skybox,TexCoords);"
                                   "}\0";

const char* vertexShadowSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 pos;\n"
                                 "\n"
                                 "uniform mat4 lightSpaceMatrix;\n"
                                 "uniform mat4 model;\n"
                                 "\n"
                                 "void main(){\n"
                                 "    gl_Position = lightSpaceMatrix * model * vec4(pos, 1.0);\n"
                                 "}";

const char* fragmentShadowSource = "#version 330 core\n"
                                   "\n"
                                   "void main(){\n"
                                   "    //vacuous function\n"
                                   "}";

// Skyboxes
std::vector<std::string> skyNight{
        "../data/textures/night/night_posx.png",
        "../data/textures/night/night_negx.png",
        "../data/textures/night/night_posy.png",
        "../data/textures/night/night_negy.png",
        "../data/textures/night/night_posz.png",
        "../data/textures/night/night_negz.png"
};

std::vector<std::string> skyMoire{
        "../data/textures/galaxy/px.png",
        "../data/textures/galaxy/nx.png",
        "../data/textures/galaxy/py.png",
        "../data/textures/galaxy/ny.png",
        "../data/textures/galaxy/pz.png",
        "../data/textures/galaxy/nz.png"
};

std::vector<std::string> skySea{
        "../data/textures/LOG/right.jpg",
        "../data/textures/LOG/left.jpg",
        "../data/textures/LOG/top.jpg",
        "../data/textures/LOG/bottom.jpg",
        "../data/textures/LOG/front.jpg",
        "../data/textures/LOG/back.jpg",
};

std::vector<std::string> skyField{
        "../data/textures/field/px.jpg",
        "../data/textures/field/nx.jpg",
        "../data/textures/field/py.jpg",
        "../data/textures/field/ny.jpg",
        "../data/textures/field/pz.jpg",
        "../data/textures/field/nz.jpg"
};

int main(){
    // Set up windowing system
    GLFWwindow* window;

    if(!glfwInit()){
        std::cerr << "GLFW initialization failed." << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORAWRD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "rw2873, 6533 Intro to CG, Silva: Homework 3", NULL, NULL);
    if(!window){
        glfwTerminate();
        std::cerr << "Window creation failed." << std::endl;
        return -1;
    }

    glfwMakeContextCurrent(window);

#ifndef __APPLE__
    glewExperimental = true;
    GLenum err = glewInit();
    if(GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
    glGetError(); // pull and savely ignonre unhandled errors like GL_INVALID_ENUM
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
#endif

    // Compile Shaders
    Shader shaderPhongProgram(vertexPhongSource, fragmentPhongSource);
    Shader shaderReflectionProgram(vertexReflectionSource, fragmentReflectionSource);
    Shader shaderSkyboxProgram(vertexSkyboxSource, fragmentSkyboxSource);
    Shader shaderShadowMappingProgram(vertexShadowSource, fragmentShadowSource);

    // Load in & establish meshes
    Mesh SM_Bunny("../data/meshes/bunny.off");
    Mesh SM_Cube("../data/meshes/cube.off");
    Mesh SM_BumpyCube("../data/meshes/bumpy_cube.off");
    Mesh SM_Quad("../data/meshes/quad.off");

    // Set up CPU-side mesh buffers
    meshes.push_back(SM_Cube);
    meshes.push_back(SM_BumpyCube);
    meshes.push_back(SM_Bunny);
    meshes.push_back(SM_Quad);

    // Cubemap
    // Other skyboxes generated with https://jaxry.github.io/panorama-to-cubemap/

    glActiveTexture(GL_TEXTURE0);
    GLuint cubemapTexture1 = loadCubemap(skyNight, true);

    glActiveTexture(GL_TEXTURE1);
    GLuint cubemapTexture2 = loadCubemap(skySea, false);

    glActiveTexture(GL_TEXTURE2);
    GLuint cubemapTexture3 = loadCubemap(skyMoire, true);

    glActiveTexture(GL_TEXTURE3);
    GLuint cubemapTexture4 = loadCubemap(skyField, true);

    skies = {cubemapTexture1, cubemapTexture2, cubemapTexture4, cubemapTexture3};

    rainbow = { COLOR_CYAN, COLOR_BLUE, COLOR_PURPLE,
                                       COLOR_MAGENTA, COLOR_RED, COLOR_ORANGE,
                                       COLOR_YELLOW, COLOR_GREEN };

    // Shadows
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0, 1.0, 1.0, 1.0};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    shaderShadowMappingProgram.use();
    shaderShadowMappingProgram.setUniformInt("shadowMap", 0);

    shaderPhongProgram.use();
    shaderPhongProgram.setUniformInt("shadowMap", 0);

    glm::mat4 lightProjection, lightView;
    glm::mat4 lightSpaceMatrix;
    float near_plane = 1.0f, far_plane = 50.0f;

    // Enables
    glEnable(GL_DEPTH_TEST);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    while(!glfwWindowShouldClose(window)){
        updateTimeBased();

        // Clear the draw buffers
        glClearColor(0.0f,0.0f,0.0f,0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // render depth of scene to texture from light's perspective
        lightProjection = glm::perspective(glm::radians(45.0f),
                                           SHADOW_ASPECT,
                                           near_plane, far_plane);
        glm::vec3 lightRight = glm::normalize(glm::cross(sceneLight._position, glm::vec3(0.0,1.0,0.0)));
        glm::vec3 lightUp = glm::normalize(glm::cross(lightRight, sceneLight._position));
        lightView = glm::lookAt(sceneLight._position, glm::vec3(0.0f), lightUp);
        lightSpaceMatrix = lightProjection * lightView;

        shaderShadowMappingProgram.use();
        shaderShadowMappingProgram.setUniformMat4("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0,0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        //glCullFace(GL_FRONT);
        // render scene
        for(const auto& obj : scene){
            shaderShadowMappingProgram.setUniformMat4("model", obj.mTransform);
            switch (obj.mClass) {
                case cube:
                    SM_Cube.draw(shaderShadowMappingProgram);
                    break;
                case bumpyCube:
                    SM_BumpyCube.draw(shaderShadowMappingProgram);
                    break;
                case bunny:
                    SM_Bunny.draw(shaderShadowMappingProgram);
                    break;
                case quad:
                    SM_Quad.draw(shaderShadowMappingProgram);
                default:
                    break;
            }
        }
        // reset all state variables
        //glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Have objects draw themselves into the draw buffer
        for(const auto& obj : scene){
            // if material is chrome
            if(obj.mMaterial.reflective){
                shaderReflectionProgram.use();
                shaderReflectionProgram.setUniformMat4("view", camera.viewMatrix);
                shaderReflectionProgram.setUniformMat4("proj", camera.projectionMatrix);
                shaderReflectionProgram.setUniformMat4("model",obj.mTransform);
                shaderReflectionProgram.setUniformVec3("viewPos", camera.cameraPos);
                shaderReflectionProgram.setUniformInt("skybox", currSky);
                glUniform3fv(glGetUniformLocation(shaderReflectionProgram.programID, "light.position"), 1, &sceneLight._position[0]);
                glUniform3fv(glGetUniformLocation(shaderReflectionProgram.programID, "light.ambient"), 1, &sceneLight._ambient[0]);
                glUniform3fv(glGetUniformLocation(shaderReflectionProgram.programID, "light.diffuse"), 1, &sceneLight._diffuse[0]);
                glUniform3fv(glGetUniformLocation(shaderReflectionProgram.programID, "light.specular"), 1, &sceneLight._specular[0]);
                shaderReflectionProgram.setUniformFloat("screenGamma", SCREEN_GAMMA);
                switch (obj.mClass) {
                    case cube:
                        SM_Cube.draw(shaderReflectionProgram);
                        break;
                    case bumpyCube:
                        SM_BumpyCube.draw(shaderReflectionProgram);
                        break;
                    case bunny:
                        SM_Bunny.draw(shaderReflectionProgram);
                        break;
                    case quad:
                        SM_Quad.draw(shaderReflectionProgram);
                    default:
                        break;
                }
            }
            // material is rendered with (Blinn-)Phong Lighting Model
            else {
                shaderPhongProgram.use();
                glUniformMatrix4fv(glGetUniformLocation(shaderPhongProgram.programID,"model"), 1, GL_FALSE, &obj.mTransform[0][0]);
                glUniformMatrix4fv(glGetUniformLocation(shaderPhongProgram.programID, "view"), 1, GL_FALSE, &camera.viewMatrix[0][0]);
                glUniformMatrix4fv(glGetUniformLocation(shaderPhongProgram.programID, "proj"), 1, GL_FALSE, &camera.projectionMatrix[0][0]);
                glUniform3fv(glGetUniformLocation(shaderPhongProgram.programID, "light.position"), 1, &sceneLight._position[0]);
                glUniform3fv(glGetUniformLocation(shaderPhongProgram.programID, "light.ambient"), 1, &sceneLight._ambient[0]);
                glUniform3fv(glGetUniformLocation(shaderPhongProgram.programID, "light.diffuse"), 1, &sceneLight._diffuse[0]);
                glUniform3fv(glGetUniformLocation(shaderPhongProgram.programID, "light.specular"), 1, &sceneLight._specular[0]);
                glUniform3fv(glGetUniformLocation(shaderPhongProgram.programID, "material.ambient"), 1, &obj.mMaterial.ambient[0]);
                glUniform3fv(glGetUniformLocation(shaderPhongProgram.programID, "material.diffuse"), 1, &obj.mMaterial.diffuse[0]);
                glUniform3fv(glGetUniformLocation(shaderPhongProgram.programID, "material.specular"), 1, &obj.mMaterial.specular[0]);
                glUniform1f(glGetUniformLocation(shaderPhongProgram.programID, "material.shininess"), obj.mMaterial.shininess);
                shaderPhongProgram.setUniformVec3("viewPos", camera.cameraPos);
                shaderPhongProgram.setUniformBool("withBlinn", WITH_BLINN);
                shaderPhongProgram.setUniformBool("highlightShadows", HIGHLIGHT_SHADOWS);
                shaderPhongProgram.setUniformFloat("screenGamma", SCREEN_GAMMA);
                shaderPhongProgram.setUniformMat4("lightSpaceMatrix", lightSpaceMatrix);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, depthMap);
                switch (obj.mClass) {
                    case cube:
                        SM_Cube.draw(shaderPhongProgram);
                        break;
                    case bumpyCube:
                        SM_BumpyCube.draw(shaderPhongProgram);
                        break;
                    case bunny:
                        SM_Bunny.draw(shaderPhongProgram);
                        break;
                    case quad:
                        SM_Quad.draw(shaderPhongProgram);
                    default:
                        break;
                }
            }
        }

        // Draw skybox
        glDepthFunc(GL_LEQUAL);
        shaderSkyboxProgram.use();
        shaderSkyboxProgram.setUniformInt("skybox", currSky);
        glm::mat4 view = glm::mat4(glm::mat3(camera.viewMatrix));
        shaderSkyboxProgram.setUniformMat4("viewMat", view);
        shaderSkyboxProgram.setUniformMat4("projMat", camera.projectionMatrix);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skies[currSky]);
        SM_Cube.draw(shaderSkyboxProgram);
        glDepthFunc(GL_LESS);

        glfwSwapBuffers(window);
        glfwPollEvents();

    }
    SM_Quad.terminate();
    SM_Cube.terminate();
    SM_BumpyCube.terminate();
    SM_Bunny.terminate();
    shaderPhongProgram.terminate();
    glfwTerminate();

    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(action == GLFW_RELEASE) return;

    switch(key){
        // Exit Program
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GL_TRUE);
            break;

        case GLFW_KEY_R:
            if(mods == GLFW_MOD_SHIFT){
                if(OBJECT_SELECTED)
                    scene[ACTIVE_OBJECT].mMaterial.reset();
                else
                    sceneLight.reset();
            }
            else if(mods == GLFW_MOD_CONTROL) {
                if (OBJECT_SELECTED)
                    scene[ACTIVE_OBJECT].mMaterial.reset();
            }
            else
                camera.resetCamera();
            break;

        // Render States
        case GLFW_KEY_CAPS_LOCK:
            // deprecated for assignment 4
            // Only Phong shading required
            // todo add PBR ?
            /*
            switch(RENDER_STATE){
                case wireframe:
                    RENDER_STATE = solid;
                    break;
                case solid:
                    RENDER_STATE = phong;
                    break;
                case phong:
                    RENDER_STATE = wireframe;
                    break;
                default:
                    break;
            }*/
            break;
        // Assignment 4 render states
        case GLFW_KEY_B:
            WITH_BLINN = !WITH_BLINN;
            break;

        case GLFW_KEY_C:
            if(OBJECT_SELECTED){
                scene[ACTIVE_OBJECT].mMaterial.updateColor(rainbow);
            }
            if(mods == GLFW_MOD_SHIFT){
                sceneLight.changeColor(rainbow);
            }
            break;

        case GLFW_KEY_X:
            if(OBJECT_SELECTED){
                scene[ACTIVE_OBJECT].mMaterial.toggleChrome();
            }
            break;

        // Toggle Camera State
        case GLFW_KEY_TAB:
            camera.updateProjState();
            break;

        // Maniplulate Camera
        case GLFW_KEY_W:
            camera.moveUp();
            break;
        case GLFW_KEY_A:
            camera.moveLeft();
            break;
        case GLFW_KEY_S:
            if(mods == GLFW_MOD_SHIFT)
                HIGHLIGHT_SHADOWS = !HIGHLIGHT_SHADOWS;
            else
                camera.moveDown();
            break;
        case GLFW_KEY_D:
            camera.moveRight();
            break;
        case GLFW_KEY_MINUS:
            camera.zoomOut();
            break;
        case GLFW_KEY_EQUAL:
            camera.zoomIn();
            break;

        // Add Objects
        case GLFW_KEY_1:
            scene.emplace_back(cube, COLOR_GRAY, 3.0f);
            scene[scene.size()-1].setWorldMin(meshes[0].getMinPoint());
            scene[scene.size()-1].setWorldMax(meshes[0].getMaxPoint());
            break;
        case GLFW_KEY_2:
            scene.emplace_back(bumpyCube, COLOR_GRAY, 3.0f);
            scene[scene.size()-1].setWorldMin(meshes[1].getMinPoint());
            scene[scene.size()-1].setWorldMax(meshes[1].getMaxPoint());
            break;
        case GLFW_KEY_3:
            scene.emplace_back(bunny, COLOR_GRAY, 1.0f);
            scene[scene.size()-1].setWorldMin(meshes[2].getMinPoint());
            scene[scene.size()-1].setWorldMax(meshes[2].getMaxPoint());
            break;
        case GLFW_KEY_4:
            scene.emplace_back(quad, COLOR_GRAY, 1.0f);
            scene[scene.size()-1].setWorldMin(meshes[3].getMinPoint());
            scene[scene.size()-1].setWorldMax(meshes[3].getMaxPoint());
            scene[scene.size()-1].mTransform = glm::scale(scene[scene.size()-1].mTransform,
                                                          glm::vec3(-10.0f,10.0f,-10.0f));
            scene[scene.size()-1].mTransform[3] = glm::vec4(0.0f,4.0f,0.0f,1.0f);
            break;
        case GLFW_KEY_T:
            testScene();
            break;

        // Object transformation
        // Default positive, hold shift for negative
        case GLFW_KEY_U: // X translation
            if(OBJECT_SELECTED){
                mods != GLFW_MOD_SHIFT ?
                  scene[ACTIVE_OBJECT].mTransform
                        = glm::translate(scene[ACTIVE_OBJECT].mTransform,
                                         glm::vec3(0.2f,0.0f,0.0f))
                : scene[ACTIVE_OBJECT].mTransform
                        = glm::translate(scene[ACTIVE_OBJECT].mTransform,
                                         glm::vec3(-0.2f,0.0f,0.0f));
            }
            break;
        case GLFW_KEY_I: // Y translation
            if(OBJECT_SELECTED){
                mods != GLFW_MOD_SHIFT ?
                  scene[ACTIVE_OBJECT].mTransform
                        = glm::translate(scene[ACTIVE_OBJECT].mTransform,
                                         glm::vec3(0.0f,0.2f,0.0f))
                : scene[ACTIVE_OBJECT].mTransform
                        = glm::translate(scene[ACTIVE_OBJECT].mTransform,
                                         glm::vec3(0.0f,-0.2f,0.0f));
            }
            break;
        case GLFW_KEY_O: // Z translation
            if(OBJECT_SELECTED){
                mods != GLFW_MOD_SHIFT ?
                  scene[ACTIVE_OBJECT].mTransform
                        = glm::translate(scene[ACTIVE_OBJECT].mTransform,
                                         glm::vec3(0.0f,0.0f,-0.2f))
                : scene[ACTIVE_OBJECT].mTransform
                        = glm::translate(scene[ACTIVE_OBJECT].mTransform,
                                         glm::vec3(0.0f,0.0f,0.2f));
            }
            break;
        case GLFW_KEY_J: // X rotation
            if(OBJECT_SELECTED){
                mods != GLFW_MOD_SHIFT ?
                 scene[ACTIVE_OBJECT].mTransform
                        = glm::rotate(scene[ACTIVE_OBJECT].mTransform,
                                      glm::radians(10.0f),
                                      glm::vec3(1.0f,0.0f,0.0f))
               : scene[ACTIVE_OBJECT].mTransform
                        = glm::rotate(scene[ACTIVE_OBJECT].mTransform,
                                      glm::radians(-10.0f),
                                      glm::vec3(1.0f,0.0f,0.0f));
            }
            break;
        case GLFW_KEY_K: // Y rotation
            if(OBJECT_SELECTED){
                mods != GLFW_MOD_SHIFT ?
                  scene[ACTIVE_OBJECT].mTransform
                        = glm::rotate(scene[ACTIVE_OBJECT].mTransform,
                                      glm::radians(10.0f),
                                      glm::vec3(0.0f,1.0f,0.0f))
                : scene[ACTIVE_OBJECT].mTransform
                        = glm::rotate(scene[ACTIVE_OBJECT].mTransform,
                                      glm::radians(-10.0f),
                                      glm::vec3(0.0f,1.0f,0.0f));
            }
            break;
        case GLFW_KEY_L: // Z rotation
            if(OBJECT_SELECTED){
                mods != GLFW_MOD_SHIFT ?
                  scene[ACTIVE_OBJECT].mTransform
                        = glm::rotate(scene[ACTIVE_OBJECT].mTransform,
                                      glm::radians(10.0f),
                                      glm::vec3(0.0f,0.0f,1.0f))
                : scene[ACTIVE_OBJECT].mTransform
                        = glm::rotate(scene[ACTIVE_OBJECT].mTransform,
                                      glm::radians(-10.0f),
                                      glm::vec3(0.0f,0.0f,1.0f));
            }
            break;
        case GLFW_KEY_SEMICOLON: // Scale
            if(OBJECT_SELECTED){
                mods != GLFW_MOD_SHIFT ?
                  scene[ACTIVE_OBJECT].mTransform
                        = glm::scale(scene[ACTIVE_OBJECT].mTransform,
                                     glm::vec3(1.2f))
                : scene[ACTIVE_OBJECT].mTransform
                        = glm::scale(scene[ACTIVE_OBJECT].mTransform,
                                     glm::vec3(0.8f));
            }
            break;

        case GLFW_KEY_BACKSPACE:
            scene.pop_back();
            break;

        case GLFW_KEY_DELETE:
            if(OBJECT_SELECTED){
                scene.erase(scene.begin() + ACTIVE_OBJECT);
                OBJECT_SELECTED = !OBJECT_SELECTED;
            }
            break;

        case GLFW_KEY_GRAVE_ACCENT:
            if(!LIGHT_MOTION){
                t_start = std::chrono::high_resolution_clock::now();
            }
            LIGHT_MOTION = !LIGHT_MOTION;
            break;

        // Change skyboxes
        case GLFW_KEY_Z:
            currSky = (++currSky) % skies.size();
            break;

        default:
            break;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;
    camera.updateAspectRatio(width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, & yPos);
    glm::vec2 screenCoords (xPos, yPos);

    if(OBJECT_SELECTED && action == GLFW_PRESS){
        scene[ACTIVE_OBJECT].mSelected = false;
    }

    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
        glm::vec3 rayDir = createRay(screenCoords);
        int i = 0;
        for(Scene_Object& obj : scene){
            if(obj.isHit(camera.cameraPos,rayDir,meshes)) {
                OBJECT_SELECTED = true;
                ACTIVE_OBJECT = i;
                obj.mSelected = true;
                break;
            }
            else{
                obj.mSelected = false;
                OBJECT_SELECTED = false;
            }
            ++i;
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    float scalingFactor = 10.0f;
    if(OBJECT_SELECTED){
        float currShine = scene[ACTIVE_OBJECT].mMaterial.shininess;
        currShine += scalingFactor * (float)yoffset;
        scene[ACTIVE_OBJECT].mMaterial.shininess =
                glm::clamp(static_cast<float>(currShine), 0.1f, 5000.0f);
    }
    else {
        LIGHT_SPEED += (float) yoffset / scalingFactor;
        if (LIGHT_SPEED > 15.0f) LIGHT_SPEED = 15.0f;
        if (LIGHT_SPEED <= 0.0f) LIGHT_SPEED = 0.05;
    }
}

glm::vec3 createRay(glm::vec2 screenCoords) {
    // Convert screen to world
    GLfloat xWorld = (screenCoords.x / ((float)camera.screenWidth) * 2) - 1;
    GLfloat yWorld = (((float)camera.screenHeight - screenCoords.y - 1) / ((float)camera.screenHeight) * 2) - 1;
    // Apply inverse transform matrices
    glm::vec4 realWorld = glm::inverse(camera.projectionMatrix * camera.viewMatrix) * glm::vec4(xWorld, yWorld, 1.0f, 1.0);
    // have to use 1.0f as z depth to get unit triangle position.

    glm::vec3 dir = glm::normalize(glm::vec3(realWorld));
    return dir;
}

void updateTimeBased(){
    // Update the time variable for time-dependent calls
    auto t_now = std::chrono::high_resolution_clock::now();
    GLfloat time = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();

    // Update light position if light is in motion.
    if(LIGHT_MOTION || TEST_RUNNING) {
        sceneLight._position.x = LIGHT_START.x + (5.0f * cos(time / LIGHT_SPEED));
        sceneLight._position.z = LIGHT_START.z + (5.0f * sin(time / LIGHT_SPEED));

        if(TEST_RUNNING) {
            scene[1].mTransform = glm::rotate(scene[1].mTransform,
                                              glm::radians(time/180.0f),
                                              glm::vec3(cos(time/LIGHT_SPEED),
                                                        sin(time/LIGHT_SPEED),
                                                        cos(time/(2*LIGHT_SPEED))));

            for (int i = 2; i < 6; ++i) {
                scene[i].mTransform[3] = glm::vec4(3.0 * sin(i * glm::radians(90.0f) + time), 1.0,
                                                   3.0 * cos(i * glm::radians(90.0f) + time), 1.0);
            }
        }
    }
}

GLuint loadTexture(char const* path){
    // Based on learnopengl.com
    GLuint texID;
    glGenTextures(1, &texID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if(data){
        GLenum format;
        switch(nrComponents){
            case 1:
                format = GL_RED;
                break;
            case 2:
                std::cout << "Your image has two intensity channels. I don't know how to feel about this." <<std::endl;
                break;
            case 3:
                format = GL_RGB;
                break;
            case 4:
                format = GL_RGBA;
                break;
            default:
                break;
        }

        glBindTexture(GL_TEXTURE_2D, texID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else{
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return texID;
}

GLuint loadCubemap(std::vector<std::string> faces, bool flip){
    // Based on learnopengl.com
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    stbi_set_flip_vertically_on_load(flip);
    int width, height, nrChannels;
    for (GLuint i = 0; i < faces.size(); ++i){
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if(data){
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else{
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    return texID;
}

void testScene(){
    if(TEST_RUNNING){
        scene.clear();
        sceneLight.reset();
        TEST_RUNNING = !TEST_RUNNING;
    }
    else {
        scene.clear();
        sceneLight.reset();
        TEST_RUNNING = !TEST_RUNNING;

        scene.emplace_back(quad, COLOR_GRAY, 1.0f);
        scene[scene.size() - 1].setWorldMin(meshes[3].getMinPoint());
        scene[scene.size() - 1].setWorldMax(meshes[3].getMaxPoint());

        scene.emplace_back(bumpyCube, COLOR_GRAY, 1.0f);
        scene[scene.size() - 1].setWorldMin(meshes[3].getMinPoint());
        scene[scene.size() - 1].setWorldMax(meshes[3].getMaxPoint());

        for (int i = 0; i < 4; ++i) {
            scene.emplace_back(bunny, COLOR_GRAY, 1.0f);
            scene[scene.size() - 1].setWorldMin(meshes[3].getMinPoint());
            scene[scene.size() - 1].setWorldMax(meshes[3].getMaxPoint());
        }

        for (int i = 0; i < 4; ++i) {
            scene.emplace_back(cube, COLOR_GRAY, 1.0f);
            scene[scene.size() - 1].setWorldMin(meshes[3].getMinPoint());
            scene[scene.size() - 1].setWorldMax(meshes[3].getMaxPoint());
        }

        // set the scene
        // move the quad to the correct position
        scene[0].mTransform = glm::scale(scene[0].mTransform, glm::vec3(100.0));
        scene[0].mTransform[3] = glm::vec4(0.0, 49.5, 0.0, 1.0);
        scene[0].mMaterial.updateColor(COLOR_GREEN);

        // Hover the bumpy cube and set it on its corner
        scene[1].mTransform[3] = glm::vec4(0.0, 3.0, 0.0, 1.0);
        scene[1].mTransform = glm::scale(scene[1].mTransform, glm::vec3(2.0f));
        scene[1].mTransform = glm::rotate(scene[1].mTransform,
                                          glm::radians(45.0f),
                                          glm::vec3(1.0, 0.0, 1.0));
        scene[1].mMaterial.reflective = true;

        // Bunnie modifcation
        for(int i = 2; i < 6; ++i){
            scene[i].mTransform[3] = glm::vec4(3.0*cos(glm::radians(i*90.0f)),1.0,3.0*sin(glm::radians(i*90.0f)),1.0);
            scene[i].mMaterial.updateColor(rainbow[i-1]);
        }

        // Move the cubes to the corners
        scene[6].mTransform[3] = glm::vec4(-20.0, 3.0, -20, 1.0);
        scene[7].mTransform[3] = glm::vec4(20.0, 3.0, -20, 1.0);
        scene[8].mTransform[3] = glm::vec4(-20.0, 3.0, 20.0, 1.0);
        scene[9].mTransform[3] = glm::vec4(20.0, 3.0, 20.0, 1.0);

        for(int i = 6; i < 10; ++i){
            scene[i].mTransform = glm::scale(scene[i].mTransform, glm::vec3(10.0f));
            scene[i].mMaterial.updateColor(rainbow[(i-2)]);
        }
    }
}