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
uniform bool blinn;

uniform vec4 Ambient; // Global ambient contribution.

uniform sampler2D diffuseSampler;
uniform sampler2D lutSampler;

layout (location=0) out vec4 out_color;

void main()
{
    // Compute the emissive term.
    vec4 Emissive = MaterialEmissive;

    // Compute the diffuse term.
    vec4 N = normalize( v2f_normalW );
    vec4 L = normalize( LightPosW - v2f_positionW );
    float NdotL = max( dot( N, L ), 0.0 );
    
    // Compute the specular term.
    vec4 V = normalize( EyePosW - v2f_positionW );

	vec4 Specular;
	vec4 Diffuse;
	if(blinn)
	{
		//Blinn Phong Model
		vec4 H = normalize( L + V );
		float NdotH = max( dot( N, H ), 0);
		vec2 uv = vec2(NdotL, NdotH);

		vec4 lut = texture2D(lutSampler, uv);
        Specular = lut.a*MaterialSpecular*LightColor;
		Diffuse = lut*MaterialDiffuse;
		out_color = ( Emissive + Ambient + Diffuse + Specular ) * texture( diffuseSampler, v2f_texcoord );
	}
    else {
		//Phong model
		vec4 R = reflect( -L, N );
		float RdotV = max( dot( R, V ), 0 );
		Specular = pow( RdotV, MaterialShininess )*MaterialSpecular;
		Diffuse = NdotL*MaterialDiffuse;
		out_color = ( Emissive + Ambient + (Diffuse + Specular)*LightColor ) * texture( diffuseSampler, v2f_texcoord );
	}
}