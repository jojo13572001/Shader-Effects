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
uniform bool enableEarthNormalMap;

uniform vec4 Ambient; // Global ambient contribution.

uniform sampler2D diffuseSampler;
uniform sampler2D lutDiffuseSampler;
uniform sampler2D lutSpecularSampler;
uniform sampler2D normalMapSampler;
float shininess;

layout (location=0) out vec4 out_color;

mat4 rotationX( in float angle ) {
	return mat4(	1.0,		0,			0,			0,
			 		0, 	cos(angle),	-sin(angle),		0,
					0, 	sin(angle),	 cos(angle),		0,
					0, 			0,			  0, 		1);
}

mat4 rotationY( in float angle ) {
	return mat4(	cos(angle),		0,		sin(angle),	0,
			 				0,		1.0,			 0,	0,
					-sin(angle),	0,		cos(angle),	0,
							0, 		0,				0,	1);
}

mat4 rotationZ( in float angle ) {
	return mat4(	cos(angle),		-sin(angle),	0,	0,
			 		sin(angle),		cos(angle),		0,	0,
							0,				0,		1,	0,
							0,				0,		0,	1);
}

vec4 calculateNormalMapN() {
	vec4 shift = texture( normalMapSampler, v2f_texcoord )*2.0 -1; //normalize from 0-1 to -1-1
	//workaround to adjust the normalmap texture to our world coordinate
	vec4 rotate = rotationX(2.0/180.0)*rotationY(-45.0/180.0)*vec4(normalize(shift.rgb), 1);
	shininess =  MaterialShininess*30;
	return normalize(vec4(rotate.rgb,0));
}

void main()
{
	shininess = MaterialShininess;
    // Compute the emissive term.
    vec4 Emissive = MaterialEmissive;

    // Compute the diffuse term.
    vec4 L = normalize(LightPosW - v2f_positionW);
    float NdotL = max( dot( normalize(v2f_normalW), L ), 0.0 );
    
    // Compute the camera view direction term.
    vec4 V = normalize( EyePosW - v2f_positionW );
	
	// Adjust normal map normal
	vec4 N = enableEarthNormalMap ? calculateNormalMapN() : normalize(v2f_normalW);
	
	vec4 Diffuse;
	vec4 Specular;
	if (shaderType == 0) {//phong
		vec4 R = reflect( -L, N );
		float RdotV = max( dot( R, V ), 0 );
		Diffuse = NdotL*MaterialDiffuse;
		Specular = pow( RdotV, shininess)*MaterialSpecular;
		out_color = ( Emissive + Ambient + (Diffuse + Specular)*LightColor ) * texture( diffuseSampler, v2f_texcoord );
	}
	else if(shaderType == 1) {//blinn
		vec4 H = normalize( L + V );
		float NdotH = max( dot( N, H ), 0);
		Diffuse = NdotL*MaterialDiffuse;
		Specular = pow( NdotH, shininess )*MaterialSpecular;
		out_color = ( Emissive + Ambient + (Diffuse + Specular)*LightColor ) * texture( diffuseSampler, v2f_texcoord );
	}
	else if(shaderType == 2) {//blinn with LUT (not support normal map)
		vec4 H = normalize( L + V );
		float NdotH = max( dot( normalize(v2f_normalW), H ), 0);
		vec2 uv = vec2(NdotL, NdotH);
		//Diffuse and Specular vector each element range between 0-1
		Diffuse = texture2D(lutDiffuseSampler, uv);
		uv = vec2(NdotH, 0);
		Specular = texture2D(lutSpecularSampler, uv);
		// out_color vector range 0-1
		out_color = ( Emissive + Ambient + (Diffuse + Specular)*LightColor ) * texture( diffuseSampler, v2f_texcoord );
	}
}