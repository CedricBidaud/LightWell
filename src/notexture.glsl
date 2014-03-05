#if defined(VERTEX)
uniform mat4 Projection;
uniform mat4 View;
uniform vec3 Position;

in vec3 VertexPosition;
//~ in vec3 VertexColor;

//~ out vec3 vPos;

void main(void){	
	//~ vColor = VertexColor;
	
	gl_Position = Projection * View * vec4(VertexPosition + Position, 1.0);
	//~ vPos = Position;
}

#endif



#if defined(FRAGMENT)

uniform vec3 UColor;

//~ in vec3 vPos;
out vec4 Color;

void main(void)
{
	//~ Color = vec4(vColor, 1.0);
	Color = vec4(UColor, 1.0);
	//~ Color = vec4(vPos.x,0.0,0.0, 1.0);
	//~ Color = vec4(1.0,0.0,1.0, 1.0);
}

#endif
