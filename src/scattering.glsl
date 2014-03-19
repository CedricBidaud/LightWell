
#if defined(VERTEX)

in vec2 VertexPosition;

out vec2 uv;

void main(void)
{	
	uv = VertexPosition * 0.5 + 0.5;
	gl_Position = vec4(VertexPosition.xy, 0.0, 1.0);
}

#endif

#if defined(FRAGMENT)

in vec2 uv;

uniform sampler2D FirstPass;
uniform vec3 LightPosition;
uniform mat4 WorldToScreen;

uniform float exposure = 1.0;
uniform float decay = 0.9;
uniform float density = 1.0;
uniform float weight = 1.0;

const int NUM_SAMPLES = 400;

out vec4  Color;

void main(void)
{
	//~ float dist = length(gl_FragCoord.xy);
	vec4 firstPass = texture(FirstPass, uv).rgba;
	vec2 lightScreenPos = (WorldToScreen * vec4(LightPosition, 1.0)).xy; // CHECKED !!
	
	//~ vec2 deltaTextCoord = vec2(gl_FragCoord.x/1024.0, gl_FragCoord.y/768.0) - lightScreenPos;
	vec2 deltaTextCoord = vec2(gl_FragCoord.x/1280.0, gl_FragCoord.y/1024.0) - lightScreenPos;
	vec2 textCoord = uv;
	
	deltaTextCoord *= 1.0 / float(NUM_SAMPLES) * density;
	
	float illuminationDecay = 1.0;
	
	int i;
	
	vec3 color = vec3(0.0);
	
	for(i = 0; i < NUM_SAMPLES; ++i){
		textCoord -= deltaTextCoord;
		vec3 sample = texture(FirstPass, textCoord).rgb;
		sample *= illuminationDecay * weight;
		color += sample;
		illuminationDecay *= decay;
	}
	
	//~ vec3 d_color = vec3(deltaTextCoord.x,deltaTextCoord.y,0.0);
	//~ vec3 d_color = vec3(lightScreenPos.x,lightScreenPos.y,0.0);
	//~ vec3 d_color = vec3(LightPosition.z,0.0,0.0);
	//~ vec3 d_color = vec3(1.0,1.0,0.0);
	
	Color = vec4(color, 1.0) * exposure;
	//~ Color = vec4(gl_FragCoord.x/1280.0, gl_FragCoord.y/1024.0, 0.0, 1.0) * exposure;
	//~ Color = firstPass;
	
}

#endif
