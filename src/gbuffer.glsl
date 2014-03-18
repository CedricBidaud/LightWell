#if defined(VERTEX)
uniform mat4 Projection;
uniform mat4 View;
uniform mat4 Object;

uniform int NbInstances;
uniform float Time;

in vec3 VertexPosition;
in vec3 VertexNormal;
in vec2 VertexTexCoord;

out vData
{
	vec2 uv;
    vec3 normal;
    vec3 position;
    float t;
}vertex;

void main(void)
{	
	vertex.uv = VertexTexCoord;
	vertex.normal = vec3(Object * vec4(VertexNormal, 1.0));; 
	vertex.position = vec3(Object * vec4(VertexPosition, 1.0));
	//~ vertex.position += vec3(gl_InstanceID, 0.0, 0.0);
	float PI = 3.1416;
	
	int nbInstances = NbInstances;
	
	int nbFloors = nbInstances / 10;
	int nbCubePerFloor = int(nbInstances / nbFloors);
	
	float rep = ((2.0*PI)/nbCubePerFloor) * (gl_InstanceID) + 0.005*gl_InstanceID;
	
	vertex.position.x += 10*cos(rep+(Time/10.0));
	vertex.position.y += 10*sin(rep+(Time/10.0));
	//~ vertex.position.x += 10*cos(rep*(1));
	//~ vertex.position.y += 10*sin(rep*(1));
	vertex.position.z += int(gl_InstanceID/nbFloors);

	gl_Position = Projection * View * vec4(vertex.position/10.0, 1.0);
}

#endif

#if defined(GEOMETRY)

#extension GL_EXT_geometry_shader4 : enable

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in vData
{
	vec2 uv;
    vec3 normal;
    vec3 position;
    float t;
}vertices[];

out fData
{
	vec2 uv;
    vec3 normal;
    vec3 position;
    float t;
}frag;

void main(void){
	int i;
	for (i = 0; i < gl_in.length(); ++i)
	{
		//gl_Position = gl_in[i].gl_Position;
		vec4 tempPos = gl_in[i].gl_Position;
		//~ if(gl_PrimitiveIDIn % 2 == 0){
			// tempPos.x += 0.3*cos(vertices[i].t);
		//~ }
		
		//~ tempPos.x *= 5.0;
		//~ tempPos.y *= 5.0;
		//~ tempPos.z *= 2.0;
		//~ tempPos.w *= 4.0;
		
		gl_Position = tempPos;

		frag.normal = vertices[i].normal;
		//~ frag.position = vertices[i].position;
		frag.position = tempPos.xyz;
		frag.uv = vertices[i].uv;
		frag.t = vertices[i].t;
		EmitVertex();
	}
}

#endif

#if defined(FRAGMENT)
uniform vec3 CameraPosition;
uniform vec3 LightPosition;
uniform mat4 WorldToScreen;

in fData
{
	vec2 uv;
    vec3 normal;
    vec3 position;
    float t;
} frag;

uniform sampler2D Diffuse;
uniform sampler2D Spec;
uniform sampler2D LightScattering;

out vec4  Color;
out vec4  Normal;

void main(void)
{
	//~ vec2 lightScreenPos = (WorldToScreen * vec4(LightPosition, 1.0)).xy; // CHECKED !!
	//~ float dist = max(0.0, 1.0 - length(lightScreenPos - vec2(gl_FragCoord.x/1024.0, gl_FragCoord.y/768.0)));
	vec3 diffuse = texture(LightScattering, frag.uv).rgb;
	float spec = texture(Spec, frag.uv).r;
	//~ Color = vec4(dist, dist, dist, 1.0);
	//~ OutColor = vec4(mix(texture(Color, uv).rgb, texture(Blur, uv).rgb, blurCoef), 1.0);
	Color = vec4(diffuse, spec);
	//~ if(dist < 0.1) Color += vec4(0.9);
	Normal = vec4(frag.normal, spec);
}

#endif
