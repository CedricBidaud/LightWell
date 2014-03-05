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
uniform float Gamma;

out vec4  OutColor;

void main(void)
{
	//~ vec4 inColor = texture(Texture1, uv);
    //~ OutColor = vec4(uv.x, uv.y, 0.0, 1.0);
    //~ OutColor = vec4(1.0, 0.0, 0.0, 1.0);
    vec4 baseColor = texture(Texture1, uv).rgba;
    vec4 correctedColor = vec4(1.0);
    correctedColor.r = pow(baseColor.r, 1.0/Gamma);
    correctedColor.g = pow(baseColor.g, 1.0/Gamma);
    correctedColor.b = pow(baseColor.b, 1.0/Gamma);
    correctedColor.a = baseColor.a;
    
    OutColor = correctedColor;
}

#endif
