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

uniform sampler2D Color;
uniform sampler2D Blur;
uniform sampler2D CoC;


out vec4  OutColor;

void main(void)
{
	float blurCoef = texture(CoC, uv).r;
	//~ vec3 baseColor = texture(Color, uv).rgb;
	//~ vec3 bluredColor = texture(Blur, uv).rgb;
	OutColor = vec4(mix(texture(Color, uv).rgb, texture(Blur, uv).rgb, blurCoef), 1.0);
    //~ OutColor = vec4(blurCoef, 0.0, 0.0, 1.0);
    //~ OutColor = vec4(bluredColor, 1.0);
}

#endif
