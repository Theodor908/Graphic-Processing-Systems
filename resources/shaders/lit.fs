#version 330 core
out vec4 FragColor;

in vec3 fragPos;
in vec3 fragNormal;
in vec2 texCoord;
in vec4 fragPosLightSpace;

#define MAX_SPOT_LIGHTS 4
in vec4 fragPosSpotSpace[MAX_SPOT_LIGHTS];

// Material
uniform sampler2D uTexture;

// Camera
uniform vec3 uViewPos;

// Directional light (sun)
uniform vec3 uSunDirection;
uniform vec3 uSunColor;
uniform float uSunIntensity;
uniform sampler2D uSunShadowMap;

// Spot lights
uniform int uNumSpotLights;
uniform vec3 uSpotPos[MAX_SPOT_LIGHTS];
uniform vec3 uSpotDir[MAX_SPOT_LIGHTS];
uniform vec3 uSpotColor[MAX_SPOT_LIGHTS];
uniform float uSpotIntensity[MAX_SPOT_LIGHTS];
uniform float uSpotCutOff[MAX_SPOT_LIGHTS];
uniform float uSpotOuterCutOff[MAX_SPOT_LIGHTS];
uniform float uSpotRange[MAX_SPOT_LIGHTS];
uniform sampler2D uSpotShadowMap[MAX_SPOT_LIGHTS];

// Point lights
#define MAX_POINT_LIGHTS 8
uniform int uNumPointLights;
uniform vec3 uPointPos[MAX_POINT_LIGHTS];
uniform vec3 uPointColor[MAX_POINT_LIGHTS];
uniform float uPointIntensity[MAX_POINT_LIGHTS];
uniform float uPointConstant[MAX_POINT_LIGHTS];
uniform float uPointLinear[MAX_POINT_LIGHTS];
uniform float uPointQuadratic[MAX_POINT_LIGHTS];

// Point light cubemap shadows
#define MAX_POINT_SHADOW_LIGHTS 3
uniform samplerCube uPointShadowMap[MAX_POINT_SHADOW_LIGHTS];
uniform float uPointFarPlane[MAX_POINT_SHADOW_LIGHTS];
uniform float uPointShadowNear;
uniform int uNumPointShadowLights;

// Ambient
uniform vec3 uAmbientColor;

float CalcShadow(vec4 fragPosLight, sampler2D shadowMap, vec3 lightDir, vec3 normal)
{
    vec3 projCoords = fragPosLight.xyz / fragPosLight.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    float currentDepth = projCoords.z;

    // Bias to reduce shadow acne
    float bias = max(0.005 * (1.0 - dot(normal, -lightDir)), 0.001);

    // PCF (percentage-closer filtering) for soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

// PCF offset directions for cubemap shadow sampling (20 samples)
const vec3 sampleOffsetDirections[20] = vec3[](
    vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
    vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
    vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
    vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
    vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0,  1, -1), vec3( 0, -1, -1)
);

float CalcPointShadow(int lightIndex, vec3 fragToLight, float currentDist, float farPlane)
{
    float shadow = 0.0;
    float bias = 0.15;
    float diskRadius = (1.0 + currentDist / farPlane) / 25.0;

    for (int s = 0; s < 20; s++) {
        // Sample the cubemap — unrolled indexing for GLSL 3.30 driver compat
        float closestDepth;
        if (lightIndex == 0)
            closestDepth = texture(uPointShadowMap[0], fragToLight + sampleOffsetDirections[s] * diskRadius).r;
        else if (lightIndex == 1)
            closestDepth = texture(uPointShadowMap[1], fragToLight + sampleOffsetDirections[s] * diskRadius).r;
        else
            closestDepth = texture(uPointShadowMap[2], fragToLight + sampleOffsetDirections[s] * diskRadius).r;

        // Linearize depth from [0,1] to world-space distance
        float near = uPointShadowNear;
        float far = farPlane;
        float linearDepth = (2.0 * near * far) / (far + near - (2.0 * closestDepth - 1.0) * (far - near));

        if (currentDist - bias > linearDepth)
            shadow += 1.0;
    }
    shadow /= 20.0;

    return shadow;
}

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(uViewPos - fragPos);
    vec3 texColor = texture(uTexture, texCoord).rgb;

    // Ambient
    vec3 ambient = uAmbientColor * texColor;

    // --- Directional light (sun) ---
    vec3 sunDir = normalize(-uSunDirection);
    float sunDiff = max(dot(normal, sunDir), 0.0);
    vec3 sunHalf = normalize(sunDir + viewDir);
    float sunSpec = pow(max(dot(normal, sunHalf), 0.0), 32.0);

    float sunShadow = CalcShadow(fragPosLightSpace, uSunShadowMap, uSunDirection, normal);

    vec3 sunResult = (1.0 - sunShadow) * (sunDiff * texColor + sunSpec * vec3(0.3))
                     * uSunColor * uSunIntensity;

    // --- Spot lights ---
    vec3 spotResult = vec3(0.0);
    for (int i = 0; i < uNumSpotLights; i++) {
        vec3 lightVec = uSpotPos[i] - fragPos;
        float dist = length(lightVec);
        vec3 lightDir = normalize(lightVec);

        // Cone attenuation
        float theta = dot(lightDir, normalize(-uSpotDir[i]));
        float epsilon = uSpotCutOff[i] - uSpotOuterCutOff[i];
        float spotAtten = clamp((theta - uSpotOuterCutOff[i]) / epsilon, 0.0, 1.0);

        // Distance attenuation
        float distAtten = clamp(1.0 - dist / uSpotRange[i], 0.0, 1.0);
        distAtten *= distAtten;

        // Diffuse + specular
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);

        float spotShadow = CalcShadow(fragPosSpotSpace[i], uSpotShadowMap[i], -lightDir, normal);

        vec3 contribution = (1.0 - spotShadow) * (diff * texColor + spec * vec3(0.3))
                           * uSpotColor[i] * uSpotIntensity[i] * spotAtten * distAtten;
        spotResult += contribution;
    }

    // --- Point lights (with cubemap shadows) ---
    vec3 pointResult = vec3(0.0);
    for (int i = 0; i < uNumPointLights; i++) {
        vec3 lightVec = uPointPos[i] - fragPos;
        float dist = length(lightVec);
        vec3 lightDir = normalize(lightVec);

        // Attenuation: 1 / (constant + linear*d + quadratic*d^2)
        float attenuation = 1.0 / (uPointConstant[i] + uPointLinear[i] * dist
                                    + uPointQuadratic[i] * dist * dist);

        // Diffuse + specular
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);

        // Cubemap shadow (for first MAX_POINT_SHADOW_LIGHTS lights)
        float pointShadow = 0.0;
        if (i < uNumPointShadowLights) {
            vec3 fragToLight = fragPos - uPointPos[i];
            pointShadow = CalcPointShadow(i, fragToLight, dist, uPointFarPlane[i]);
        }

        vec3 contribution = (1.0 - pointShadow) * (diff * texColor + spec * vec3(0.3))
                           * uPointColor[i] * uPointIntensity[i] * attenuation;
        pointResult += contribution;
    }

    FragColor = vec4(ambient + sunResult + spotResult + pointResult, 1.0);
}
