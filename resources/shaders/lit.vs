#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uSunLightSpaceMVP;

#define MAX_SPOT_LIGHTS 4
uniform mat4 uSpotLightSpaceMVP[MAX_SPOT_LIGHTS];
uniform int uNumSpotLights;

out vec3 fragPos;
out vec3 fragNormal;
out vec2 texCoord;
out vec4 fragPosLightSpace;
out vec4 fragPosSpotSpace[MAX_SPOT_LIGHTS];

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    fragPos = worldPos.xyz;
    fragNormal = mat3(transpose(inverse(uModel))) * aNormal;
    texCoord = aUV;

    fragPosLightSpace = uSunLightSpaceMVP * worldPos;

    for (int i = 0; i < uNumSpotLights; i++) {
        fragPosSpotSpace[i] = uSpotLightSpaceMVP[i] * worldPos;
    }

    gl_Position = uProjection * uView * worldPos;
}
