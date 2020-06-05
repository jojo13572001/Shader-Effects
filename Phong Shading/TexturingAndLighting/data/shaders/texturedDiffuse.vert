#version 330 core

layout(location=0) in vec3 in_position;
layout(location=2) in vec3 in_normal;
layout(location=8) in vec2 in_texcoord;

out vec4 v2f_positionW; // Position in world space.
out vec4 v2f_normalW; // Surface normal in world space.
out vec2 v2f_texcoord;

// Model, View, Projection matrix.
uniform mat4 ModelViewProjectionMatrix;
uniform mat4 ModelMatrix;

uniform sampler2D bumpMapSampler;
uniform bool enableEarthBumpMap;

void main()
{
	vec4 uv = texture2D( bumpMapSampler, in_texcoord );
	float bump = enableEarthBumpMap ? uv.r*0.15 : 0; //scale=0.15
	vec3 bump_in_position = normalize(in_normal) * bump + in_position;

    gl_Position = ModelViewProjectionMatrix * vec4(bump_in_position, 1);
    v2f_positionW = ModelMatrix * vec4(bump_in_position, 1); 
    v2f_normalW = ModelMatrix * vec4(in_normal, 0);
    v2f_texcoord = in_texcoord;
}