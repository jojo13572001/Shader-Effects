#version 330 core

in vec4 v2f_positionW; // Position in world space.
in vec4 v2f_normalW; // Surface normal in world space.
in vec2 v2f_texcoord;

uniform vec4 EyePosW;   // Eye position in world space.
uniform vec4 LightPosW; // Light's position in world space.
uniform vec4 LightColor; // Light's diffuse and specular contribution.(1,1,1,1)

uniform vec4 MaterialEmissive;
uniform vec4 MaterialDiffuse;
uniform vec4 MaterialSpecular;
uniform float MaterialShininess;
uniform int shaderType;

uniform vec4 Ambient; // Global ambient contribution.

uniform sampler2D diffuseSampler;
uniform sampler2D lutDiffuseSampler;
uniform sampler2D lutSpecularSampler;

layout (location=0) out vec4 out_color;

void main()
{
    // Compute the emissive term.
    vec4 Emissive = MaterialEmissive;

    // Compute the diffuse term.
    vec4 N = normalize( v2f_normalW );
    vec4 L = normalize( LightPosW - v2f_positionW );
    float NdotL = max( dot( N, L ), 0.0 );
    
    // Compute the camera view direction term.
    vec4 V = normalize( EyePosW - v2f_positionW );

	if (shaderType == 0) {//phong
		//Phong model
		vec4 R = reflect( -L, N );
		float RdotV = max( dot( R, V ), 0 );
		vec4 Diffuse = NdotL*MaterialDiffuse;
		vec4 Specular = pow( RdotV, MaterialShininess )*MaterialSpecular;
		out_color = ( Emissive + Ambient + (Diffuse + Specular)*LightColor ) * texture( diffuseSampler, v2f_texcoord );
	}
	else if(shaderType == 1) {//blinn
		//Blinn Phong Model
		vec4 H = normalize( L + V );
		float NdotH = max( dot( N, H ), 0);
		vec4 Diffuse = NdotL*MaterialDiffuse;
		vec4 Specular = pow( NdotH, MaterialShininess )*MaterialSpecular;
		out_color = ( Emissive + Ambient + (Diffuse + Specular)*LightColor ) * texture( diffuseSampler, v2f_texcoord );
	}
	else if(shaderType == 2) {//blinn with LUT
		//Blinn Phong Model
		vec4 H = normalize( L + V );
		float NdotH = max( dot( N, H ), 0);
		vec2 uv = vec2(NdotL, 0);
		//Diffuse and Specular vector range 0-1
		vec4 Diffuse = texture2D(lutDiffuseSampler, uv);
		uv = vec2(NdotH, 0);
		vec4 Specular = texture2D(lutSpecularSampler, uv);
		// out_color vector range 0-1
		out_color = ( Emissive + Ambient + Diffuse + Specular) * texture( diffuseSampler, v2f_texcoord );
	}
}