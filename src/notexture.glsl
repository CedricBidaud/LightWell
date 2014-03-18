#if defined(VERTEX)
uniform mat4 Projection;
uniform mat4 View;
uniform vec3 Position;

uniform int isLight;

uniform int NbInstances;
uniform float Time;

in vec3 VertexPosition;
//~ in vec3 VertexColor;

//~ out vec3 vPos;

void main(void){	
	
	vec3 tempPos = VertexPosition + Position;
	
	if(isLight == 0){
		float PI = 3.1416;
		
		int nbInstances = NbInstances;
		
		int nbFloors = nbInstances / 10;
		int nbCubePerFloor = int(nbInstances / nbFloors);
		
		float rep = ((2.0*PI)/nbCubePerFloor) * (gl_InstanceID) + 0.005*gl_InstanceID;
		
		tempPos.x += 10*cos(rep+(Time/10.0));
		tempPos.y += 10*sin(rep+(Time/10.0));
		//~ tempPos.x += 10*cos(rep*(1));
		//~ tempPos.y += 10*sin(rep*(1));
		tempPos.z += int(gl_InstanceID/nbFloors);
	}else{
		tempPos.y /= 5.0;
		tempPos.y += 20.0;
	}

	gl_Position = Projection * View * vec4(tempPos/10.0, 1.0);
	
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
