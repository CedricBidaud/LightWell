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

uniform sampler2D Material;
uniform sampler2D LightScattering;
uniform sampler2D Normal;
uniform sampler2D Depth;
uniform vec3 CameraPosition;
uniform vec3  LightPosition;
uniform vec3  LightColor;
uniform float LightIntensity;
uniform mat4 InverseViewProjection; // aka ScreenToWorld
uniform mat4 WorldToScreen;

out vec4  Color;

void main(void)
{
	//~ float dist = length(gl_FragCoord.xy);
	vec4  material = texture(Material, uv).rgba;
	vec3  normal = texture(Normal, uv).rgb;
	float depth = texture(Depth, uv).r;
	
	vec2  xy = uv * 2.0 -1.0;
	vec4  wPosition =  vec4(xy, depth * 2.0 -1.0, 1.0) * InverseViewProjection;
	vec3  position = vec3(wPosition/wPosition.w);

	vec3 diffuse = material.rgb;
	float spec = material.a;

	vec3 n = normalize(normal);
	vec3 l =  LightPosition - position;

	vec3 v = position - CameraPosition;
	vec3 h = normalize(l-v);
	float n_dot_l = clamp(dot(n, l), 0, 1.0);
	float n_dot_h = clamp(dot(n, h), 0, 1.0);

	float d = distance(LightPosition, position);
	float att = clamp(  1.0 / ( 1.0 + 1.0 * (d*d)), 0.0, 1.0);
	//~ float att = 1.0;

	vec3 color = LightColor * LightIntensity * att * (diffuse * n_dot_l + spec * vec3(1.0, 1.0, 1.0) *  pow(n_dot_h, spec * 100.0));
	
	//~ vec3 color = LightColor * LightIntensity * att;
	
	//~ vec2 lightScreenPos = (WorldToScreen * vec4(LightPosition, 1.0)).xy; // CHECKED !!
	//~ float dist = max(0.0, 1.0 - length(lightScreenPos - vec2(gl_FragCoord.x/1024.0, gl_FragCoord.y/768.0)));
	//~ color += vec3(dist);

	//~ OutColor = vec4(mix(texture(Color, uv).rgb, texture(Blur, uv).rgb, blurCoef), 1.0);

	Color = vec4(mix(color, texture(LightScattering,uv).rgb, 0.2), 1.0);
	//~ Color = vec4(dist, dist, dist, 1.0);
	//~ Color = vec4(gl_FragCoord.x/1024.0, gl_FragCoord.y/768.0, 0.0, 1.0);
	//~ Color = vec4(lightScreenPos.x, 0.0, lightScreenPos.y, 1.0);
	//~ Color = vec4(vec3(1-d), 1.0);
	//Color = vec4(depth, 0.0 , 0.0, 1.0);
	//Color = vec4(normal, 1.0);
}

#endif
