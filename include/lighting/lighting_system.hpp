#pragma once
#include "lighting/light.hpp"
#include "shaders/shader.hpp"
#include "glad.h"
#include <glm/glm.hpp>
#include <vector>
#include <functional>

class LightingSystem {
public:
    DirectionalLight sun;
    std::vector<SpotLight> spotLights;
    std::vector<PointLight> pointLights;
    glm::vec3 ambientColor = glm::vec3(0.15f, 0.15f, 0.2f);

    void Load();
    void Unload();

    void SetSun(const DirectionalLight& light);
    void AddSpotLight(const SpotLight& light);
    void AddPointLight(const PointLight& light);

    // Render shadow maps — calls drawScene for each shadow-casting light
    void RenderShadowMaps(std::function<void(unsigned int shaderID, const glm::mat4& lightMVP)> drawScene);

    // Upload all light data + bind shadow maps to the given lit shader
    void ApplyToShader(unsigned int litShaderID, const glm::vec3& cameraPos);

    Shader& GetShadowShader() { return shadowShader; }

private:
    Shader shadowShader;

    // Sun shadow map
    unsigned int sunShadowFBO = 0;
    unsigned int sunShadowMap = 0;
    glm::mat4 sunLightSpaceMatrix;

    // Spot light shadow maps
    std::vector<unsigned int> spotShadowFBOs;
    std::vector<unsigned int> spotShadowMaps;
    std::vector<glm::mat4> spotLightSpaceMatrices;

    static const int SHADOW_WIDTH = 2048;
    static const int SHADOW_HEIGHT = 2048;

    // Point light cubemap shadows
    std::vector<unsigned int> pointShadowFBOs;
    std::vector<unsigned int> pointShadowCubemaps;
    std::vector<float> pointShadowFarPlanes;

    static const int POINT_SHADOW_WIDTH = 1024;
    static const int POINT_SHADOW_HEIGHT = 1024;
    static constexpr float POINT_SHADOW_NEAR = 0.1f;

    void CreateShadowFBO(unsigned int& fbo, unsigned int& depthMap);
    void CreateCubemapShadowFBO(unsigned int& fbo, unsigned int& cubemap);
    glm::mat4 CalcSunLightSpaceMatrix();
    glm::mat4 CalcSpotLightSpaceMatrix(const SpotLight& light);
    float CalcPointLightRange(const PointLight& light);
};
