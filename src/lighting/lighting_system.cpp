#include "lighting/lighting_system.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <iostream>
#include <cmath>

void LightingSystem::Load() {
    shadowShader = Shader::LoadShader("resources/shaders/shadow.vs", "resources/shaders/shadow.fs");

    // Create sun shadow map
    CreateShadowFBO(sunShadowFBO, sunShadowMap);
}

void LightingSystem::Unload() {
    shadowShader.Unload();

    glDeleteFramebuffers(1, &sunShadowFBO);
    glDeleteTextures(1, &sunShadowMap);
    sunShadowFBO = sunShadowMap = 0;

    for (auto fbo : spotShadowFBOs) glDeleteFramebuffers(1, &fbo);
    for (auto map : spotShadowMaps) glDeleteTextures(1, &map);
    spotShadowFBOs.clear();
    spotShadowMaps.clear();
    spotLightSpaceMatrices.clear();
    spotLights.clear();

    for (auto fbo : pointShadowFBOs) glDeleteFramebuffers(1, &fbo);
    for (auto cm : pointShadowCubemaps) glDeleteTextures(1, &cm);
    pointShadowFBOs.clear();
    pointShadowCubemaps.clear();
    pointShadowFarPlanes.clear();
    pointLights.clear();
}

void LightingSystem::SetSun(const DirectionalLight& light) {
    sun = light;
}

void LightingSystem::AddPointLight(const PointLight& light) {
    if ((int)pointLights.size() >= MAX_POINT_LIGHTS) {
        std::cout << "WARNING::LIGHTING::MAX_POINT_LIGHTS reached" << std::endl;
        return;
    }
    pointLights.push_back(light);

    // Allocate cubemap shadow for the first MAX_POINT_SHADOW_LIGHTS
    if ((int)pointShadowFBOs.size() < MAX_POINT_SHADOW_LIGHTS) {
        unsigned int fbo, cubemap;
        CreateCubemapShadowFBO(fbo, cubemap);
        pointShadowFBOs.push_back(fbo);
        pointShadowCubemaps.push_back(cubemap);
        pointShadowFarPlanes.push_back(CalcPointLightRange(light));
    }
}

void LightingSystem::AddSpotLight(const SpotLight& light) {
    if ((int)spotLights.size() >= MAX_SPOT_LIGHTS) {
        std::cout << "WARNING::LIGHTING::MAX_SPOT_LIGHTS reached" << std::endl;
        return;
    }
    spotLights.push_back(light);

    unsigned int fbo, map;
    CreateShadowFBO(fbo, map);
    spotShadowFBOs.push_back(fbo);
    spotShadowMaps.push_back(map);
    spotLightSpaceMatrices.push_back(glm::mat4(1.0f));
}

void LightingSystem::CreateShadowFBO(unsigned int& fbo, unsigned int& depthMap) {
    glGenFramebuffers(1, &fbo);

    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LightingSystem::CreateCubemapShadowFBO(unsigned int& fbo, unsigned int& cubemap) {
    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    for (int face = 0; face < 6; face++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_DEPTH_COMPONENT,
                     POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT, 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    // Attach first face to validate the FBO; we'll re-attach per face during rendering
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X, cubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

float LightingSystem::CalcPointLightRange(const PointLight& light) {
    // Solve: 1/(c + l*d + q*d^2) = 5/256 for d
    float threshold = 5.0f / 256.0f;
    float c = light.constant - 1.0f / threshold;  // c - 1/threshold
    float l = light.linear;
    float q = light.quadratic;

    float discriminant = l * l - 4.0f * q * c;
    if (discriminant < 0.0f) return 50.0f; // fallback
    float range = (-l + std::sqrt(discriminant)) / (2.0f * q);
    return (range > 0.0f) ? range : 50.0f;
}

glm::mat4 LightingSystem::CalcSunLightSpaceMatrix() {
    float orthoSize = 80.0f;
    glm::mat4 lightProj = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 150.0f);

    glm::vec3 lightPos = -glm::normalize(sun.direction) * 60.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    return lightProj * lightView;
}

glm::mat4 LightingSystem::CalcSpotLightSpaceMatrix(const SpotLight& light) {
    float fov = glm::acos(light.outerCutOff) * 2.0f;
    glm::mat4 lightProj = glm::perspective(fov, 1.0f, 0.5f, light.range);

    glm::vec3 up = (glm::abs(light.direction.y) > 0.99f)
                   ? glm::vec3(0.0f, 0.0f, 1.0f)
                   : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 lightView = glm::lookAt(light.position, light.position + light.direction, up);

    return lightProj * lightView;
}

void LightingSystem::RenderShadowMaps(
    std::function<void(unsigned int shaderID, const glm::mat4& lightMVP)> drawScene)
{
    glUseProgram(shadowShader.programID);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

    // Sun shadow pass
    sunLightSpaceMatrix = CalcSunLightSpaceMatrix();
    glBindFramebuffer(GL_FRAMEBUFFER, sunShadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    drawScene(shadowShader.programID, sunLightSpaceMatrix);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Spot light shadow passes
    for (int i = 0; i < (int)spotLights.size(); i++) {
        spotLightSpaceMatrices[i] = CalcSpotLightSpaceMatrix(spotLights[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, spotShadowFBOs[i]);
        glClear(GL_DEPTH_BUFFER_BIT);
        drawScene(shadowShader.programID, spotLightSpaceMatrices[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Point light cubemap shadow passes
    glViewport(0, 0, POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT);

    // 6 cubemap face directions and up vectors
    struct CubeFace { GLenum target; glm::vec3 dir; glm::vec3 up; };
    CubeFace faces[6] = {
        { GL_TEXTURE_CUBE_MAP_POSITIVE_X, { 1, 0, 0}, {0,-1, 0} },
        { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, {-1, 0, 0}, {0,-1, 0} },
        { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, { 0, 1, 0}, {0, 0, 1} },
        { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, { 0,-1, 0}, {0, 0,-1} },
        { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, { 0, 0, 1}, {0,-1, 0} },
        { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, { 0, 0,-1}, {0,-1, 0} },
    };

    int numPointShadows = (int)pointShadowFBOs.size();
    for (int i = 0; i < numPointShadows; i++) {
        // Recalculate far plane each frame so UI attenuation changes are picked up
        pointShadowFarPlanes[i] = CalcPointLightRange(pointLights[i]);
        float farPlane = pointShadowFarPlanes[i];
        glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, POINT_SHADOW_NEAR, farPlane);
        glm::vec3 pos = pointLights[i].position;

        glBindFramebuffer(GL_FRAMEBUFFER, pointShadowFBOs[i]);
        for (int f = 0; f < 6; f++) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                   faces[f].target, pointShadowCubemaps[i], 0);
            glClear(GL_DEPTH_BUFFER_BIT);

            glm::mat4 view = glm::lookAt(pos, pos + faces[f].dir, faces[f].up);
            glm::mat4 lightMVP = proj * view;
            drawScene(shadowShader.programID, lightMVP);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Restore viewport
    glViewport(0, 0, 800, 600);
}

void LightingSystem::ApplyToShader(unsigned int litShaderID, const glm::vec3& cameraPos) {
    glUseProgram(litShaderID);

    // Camera position
    glUniform3fv(glGetUniformLocation(litShaderID, "uViewPos"), 1, glm::value_ptr(cameraPos));

    // Ambient
    glUniform3fv(glGetUniformLocation(litShaderID, "uAmbientColor"), 1, glm::value_ptr(ambientColor));

    // Sun
    glUniform3fv(glGetUniformLocation(litShaderID, "uSunDirection"), 1, glm::value_ptr(sun.direction));
    glUniform3fv(glGetUniformLocation(litShaderID, "uSunColor"), 1, glm::value_ptr(sun.color));
    glUniform1f(glGetUniformLocation(litShaderID, "uSunIntensity"), sun.intensity);
    glUniformMatrix4fv(glGetUniformLocation(litShaderID, "uSunLightSpaceMVP"),
                       1, GL_FALSE, glm::value_ptr(sunLightSpaceMatrix));

    // Bind sun shadow map to texture unit 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sunShadowMap);
    glUniform1i(glGetUniformLocation(litShaderID, "uSunShadowMap"), 1);

    // Spot lights
    int numSpots = (int)spotLights.size();
    glUniform1i(glGetUniformLocation(litShaderID, "uNumSpotLights"), numSpots);

    for (int i = 0; i < numSpots; i++) {
        std::string idx = std::to_string(i);

        glUniform3fv(glGetUniformLocation(litShaderID, ("uSpotPos[" + idx + "]").c_str()),
                     1, glm::value_ptr(spotLights[i].position));
        glUniform3fv(glGetUniformLocation(litShaderID, ("uSpotDir[" + idx + "]").c_str()),
                     1, glm::value_ptr(spotLights[i].direction));
        glUniform3fv(glGetUniformLocation(litShaderID, ("uSpotColor[" + idx + "]").c_str()),
                     1, glm::value_ptr(spotLights[i].color));
        glUniform1f(glGetUniformLocation(litShaderID, ("uSpotIntensity[" + idx + "]").c_str()),
                    spotLights[i].intensity);
        glUniform1f(glGetUniformLocation(litShaderID, ("uSpotCutOff[" + idx + "]").c_str()),
                    spotLights[i].cutOff);
        glUniform1f(glGetUniformLocation(litShaderID, ("uSpotOuterCutOff[" + idx + "]").c_str()),
                    spotLights[i].outerCutOff);
        glUniform1f(glGetUniformLocation(litShaderID, ("uSpotRange[" + idx + "]").c_str()),
                    spotLights[i].range);

        glUniformMatrix4fv(glGetUniformLocation(litShaderID,
                           ("uSpotLightSpaceMVP[" + idx + "]").c_str()),
                           1, GL_FALSE, glm::value_ptr(spotLightSpaceMatrices[i]));

        // Bind spot shadow map to texture unit 2+i
        glActiveTexture(GL_TEXTURE2 + i);
        glBindTexture(GL_TEXTURE_2D, spotShadowMaps[i]);
        glUniform1i(glGetUniformLocation(litShaderID,
                    ("uSpotShadowMap[" + idx + "]").c_str()), 2 + i);
    }

    // Point lights
    int numPoints = (int)pointLights.size();
    glUniform1i(glGetUniformLocation(litShaderID, "uNumPointLights"), numPoints);

    int numPointShadows = (int)pointShadowFBOs.size();
    glUniform1i(glGetUniformLocation(litShaderID, "uNumPointShadowLights"), numPointShadows);
    glUniform1f(glGetUniformLocation(litShaderID, "uPointShadowNear"), POINT_SHADOW_NEAR);

    for (int i = 0; i < numPoints; i++) {
        std::string idx = std::to_string(i);

        glUniform3fv(glGetUniformLocation(litShaderID, ("uPointPos[" + idx + "]").c_str()),
                     1, glm::value_ptr(pointLights[i].position));
        glUniform3fv(glGetUniformLocation(litShaderID, ("uPointColor[" + idx + "]").c_str()),
                     1, glm::value_ptr(pointLights[i].color));
        glUniform1f(glGetUniformLocation(litShaderID, ("uPointIntensity[" + idx + "]").c_str()),
                    pointLights[i].intensity);
        glUniform1f(glGetUniformLocation(litShaderID, ("uPointConstant[" + idx + "]").c_str()),
                    pointLights[i].constant);
        glUniform1f(glGetUniformLocation(litShaderID, ("uPointLinear[" + idx + "]").c_str()),
                    pointLights[i].linear);
        glUniform1f(glGetUniformLocation(litShaderID, ("uPointQuadratic[" + idx + "]").c_str()),
                    pointLights[i].quadratic);
    }

    // Bind point shadow cubemaps to texture units 6-8
    for (int i = 0; i < numPointShadows; i++) {
        std::string idx = std::to_string(i);
        glActiveTexture(GL_TEXTURE6 + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pointShadowCubemaps[i]);
        glUniform1i(glGetUniformLocation(litShaderID,
                    ("uPointShadowMap[" + idx + "]").c_str()), 6 + i);
        glUniform1f(glGetUniformLocation(litShaderID,
                    ("uPointFarPlane[" + idx + "]").c_str()), pointShadowFarPlanes[i]);
    }
}
