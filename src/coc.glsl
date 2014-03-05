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

uniform sampler2D Depth;
uniform mat4 ScreenToView;
uniform float Focus;
uniform float Near;
uniform float Far;

out vec4  Color;

void main(void)
{
	float depth = texture(Depth, uv).r;
	vec2  xy = uv * 2.0 -1.0;
	vec4  wViewPos =  ScreenToView * vec4(xy, depth * 2.0 -1.0, 1.0);
	vec3  viewPos = vec3(wViewPos/wViewPos.w);
	float viewDepth = -viewPos.z;
	
	float dist;
	
	if (viewDepth < Focus)
		dist = abs( (viewDepth - Focus) / Near );
	else
		dist = abs( (viewDepth - Focus) / Far );
	
    //~ Color = vec4(1.0, 0.0, 1.0, 1.0);
    Color = vec4(dist, 0.0, 0.0, 1.0);
}

#endif
