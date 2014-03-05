#if defined(VERTEX)

in vec2 VertexPosition;

out vec2 uv;

void main(void)
{   
    uv = (VertexPosition) * 0.5 + 0.5;
    gl_Position = vec4(VertexPosition.xy, 0.0, 1.0);
}

#endif

#if defined(FRAGMENT)

in vec2 uv;

uniform sampler2D Texture1;
uniform int SampleCount;
uniform vec2 Direction;
uniform vec2 TexelSize;

out vec4  OutColor;

void main ()
{
	//~ vec4 inColor = texture(Texture1, uv).rgba;
	vec4 blured = vec4(0.0);
	
	for(int i = 0; i < SampleCount; ++i){
		for(int j = 0; j < SampleCount; ++j){
			vec2 tempUV = uv;
			tempUV.x += i / 1024.0f;
			tempUV.y += j / 768.0f;
			blured += texture(Texture1, tempUV).rgba;
		}
	}
	
	blured /= SampleCount*SampleCount;
	
    OutColor = blured;    //~ OutColor = inColor;
}
#endif

