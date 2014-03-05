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

uniform sampler2D Texture1;

out vec4  Color;

void main(void)
{
	vec3 color = texture(Texture1, uv).rgb;
	Color = vec4(color, 1.0);
}

#endif
