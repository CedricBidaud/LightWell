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
	float gaussian[49];
	
	gaussian[0] = 0.0005588753;
	gaussian[1] = 0.0015191805;
	gaussian[2] = 0.0041295608;
	gaussian[3] = 0.0112253100;
	gaussian[4] = 0.0041295608;
	gaussian[5] = 0.0015191805;
	gaussian[6] = 0.0005588753;
	
	gaussian[7]  = 0.0015191805;
	gaussian[8]  = 0.0041295608;
	gaussian[9]  = 0.0112253100;
	gaussian[10] = 0.0305135561;
	gaussian[11] = 0.0112253100;
	gaussian[12] = 0.0041295608;
	gaussian[13] = 0.0015191805;
	
	gaussian[14] = 0.0041295608;
	gaussian[15] = 0.0112253100;
	gaussian[16] = 0.0305135561;
	gaussian[17] = 0.0829444450;
	gaussian[18] = 0.0305135561;
	gaussian[19] = 0.0112253100;
	gaussian[20] = 0.0041295608;
	
	gaussian[21] = 0.0112253100;
	gaussian[22] = 0.0305135561;
	gaussian[23] = 0.0829444450;
	gaussian[24] = 0.2254663776;
	gaussian[25] = 0.0829444450;
	gaussian[26] = 0.0305135561;
	gaussian[27] = 0.0112253100;
	
	gaussian[28] = 0.0041295608;
	gaussian[29] = 0.0112253100;
	gaussian[30] = 0.0305135561;
	gaussian[31] = 0.0829444450;
	gaussian[32] = 0.0305135561;
	gaussian[33] = 0.0112253100;
	gaussian[34] = 0.0041295608;
	
	gaussian[35] = 0.0015191805;
	gaussian[36] = 0.0041295608;
	gaussian[37] = 0.0112253100;
	gaussian[38] = 0.0305135561;
	gaussian[39] = 0.0112253100;
	gaussian[40] = 0.0041295608;
	gaussian[41] = 0.0015191805;
	
	gaussian[42] = 0.0005588753;
	gaussian[43] = 0.0015191805;
	gaussian[44] = 0.0041295608;
	gaussian[45] = 0.0112253100;
	gaussian[46] = 0.0041295608;
	gaussian[47] = 0.0015191805;
	gaussian[48] = 0.0005588753;
	
	//~ vec4 inColor = texture(Texture1, uv).rgba;
	vec3 blured = vec3(0.0);
	for(int i = -3; i <= 3; ++i){
		for(int j = -3; j <= 3; ++j){
			vec2 tempUV = uv;
			tempUV.x += i / 1024.0f;
			tempUV.y += j / 768.0f;
			
			int idx = (i+3) + (j+3)*7;
			
			blured += texture(Texture1, tempUV).rgb * gaussian[idx];
		}
	}
	
    OutColor = vec4(blured,1.0);    //~ OutColor = inColor;
}
#endif

