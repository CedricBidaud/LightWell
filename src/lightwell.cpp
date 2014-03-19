#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>

#include "glew/glew.h"
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "GL/glfw.h"
#include "stb/stb_image.h"
#include "imgui/imgui.h"
#include "imgui/imguiRenderGL3.h"

#include "glm/glm.hpp"
#include "glm/vec3.hpp" // glm::vec3
#include "glm/vec4.hpp" // glm::vec4, glm::ivec4
#include "glm/mat4x4.hpp" // glm::mat4
#include "glm/gtc/matrix_transform.hpp" // glm::translate, glm::rotate, glm::scale, glm::perspective
#include "glm/gtc/type_ptr.hpp" // glm::value_ptr

#ifndef DEBUG_PRINT
#define DEBUG_PRINT 1
#endif

#if DEBUG_PRINT == 0
#define debug_print(FORMAT, ...) ((void)0)
#else
#ifdef _MSC_VER
#define debug_print(FORMAT, ...) \
    fprintf(stderr, "%s() in %s, line %i: " FORMAT "\n", \
        __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define debug_print(FORMAT, ...) \
    fprintf(stderr, "%s() in %s, line %i: " FORMAT "\n", \
        __func__, __FILE__, __LINE__, __VA_ARGS__)
#endif
#endif

// Font buffers
extern const unsigned char DroidSans_ttf[];
extern const unsigned int DroidSans_ttf_len;    

struct ShaderGLSL
{
    enum ShaderType
    {
        VERTEX_SHADER = 1,
        FRAGMENT_SHADER = 2,
        GEOMETRY_SHADER = 4
    };
    GLuint program;
};

int compile_and_link_shader(ShaderGLSL & shader, int typeMask, const char * sourceBuffer, int bufferSize);
int destroy_shader(ShaderGLSL & shader);
int load_shader_from_file(ShaderGLSL & shader, const char * path, int typemask);
    
struct Camera
{
    float radius;
    float theta;
    float phi;
    glm::vec3 o;
    glm::vec3 eye;
    glm::vec3 up;
};

void camera_defaults(Camera & c);
void camera_zoom(Camera & c, float factor);
void camera_turn(Camera & c, float phi, float theta);
void camera_pan(Camera & c, float x, float y);

struct GUIStates
{
    bool panLock;
    bool turnLock;
    bool zoomLock;
    int lockPositionX;
    int lockPositionY;
    int camera;
    double time;
    bool playing;
    static const float MOUSE_PAN_SPEED;
    static const float MOUSE_ZOOM_SPEED;
    static const float MOUSE_TURN_SPEED;
};
const float GUIStates::MOUSE_PAN_SPEED = 0.001f;
const float GUIStates::MOUSE_ZOOM_SPEED = 0.05f;
const float GUIStates::MOUSE_TURN_SPEED = 0.005f;


void init_gui_states(GUIStates & guiStates)
{
    guiStates.panLock = false;
    guiStates.turnLock = false;
    guiStates.zoomLock = false;
    guiStates.lockPositionX = 0;
    guiStates.lockPositionY = 0;
    guiStates.camera = 0;
    guiStates.time = 0.0;
    guiStates.playing = false;
}

int main( int argc, char **argv )
{
    //~ int width = 1024, height=768;
    int width = 1280, height=1024;
    float widthf = (float) width, heightf = (float) height;
    double t;
    float fps = 0.f;

    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        exit( EXIT_FAILURE );
    }

    // Force core profile on Mac OSX
#ifdef __APPLE__
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);
    glfwOpenWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    // Open a window and create its OpenGL context
    if( !glfwOpenWindow( width, height, 0,0,0,0, 24, 0, GLFW_WINDOW ) )
    {
        fprintf( stderr, "Failed to open GLFW window\n" );

        glfwTerminate();
        exit( EXIT_FAILURE );
    }
	//~ glfwEnable(GLFW_MOUSE_CURSOR);
    glfwSetWindowTitle( "LightWell" );


    // Core profile is flagged as experimental in glew
#ifdef __APPLE__
    glewExperimental = GL_TRUE;
#endif
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
          /* Problem: glewInit failed, something is seriously wrong. */
          fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
          exit( EXIT_FAILURE );
    }

    // Ensure we can capture the escape key being pressed below
    glfwEnable( GLFW_STICKY_KEYS );

    // Enable vertical sync (on cards that support it)
    glfwSwapInterval( 1 );
    GLenum glerr = GL_NO_ERROR;
    glerr = glGetError();

    if (!imguiRenderGLInit(DroidSans_ttf, DroidSans_ttf_len))
    {
        fprintf(stderr, "Could not init GUI renderer.\n");
        exit(EXIT_FAILURE);
    }

    // Init viewer structures
    Camera camera;
    camera_defaults(camera);
    GUIStates guiStates;
    init_gui_states(guiStates);

    // GUI
    float numLights = 10.f;

    // Load images and upload textures
    GLuint textures[3];
    glGenTextures(3, textures);
    int x;
    int y;
    int comp; 
    unsigned char * diffuse = stbi_load("textures/spnza_bricks_a_diff.tga", &x, &y, &comp, 3);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, diffuse);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    fprintf(stderr, "Diffuse %dx%d:%d\n", x, y, comp);
    unsigned char * spec = stbi_load("textures/spnza_bricks_a_spec.tga", &x, &y, &comp, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, x, y, 0, GL_RED, GL_UNSIGNED_BYTE, spec);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    fprintf(stderr, "Spec %dx%d:%d\n", x, y, comp);

    // Try to load and compile shader
    int status;
    ShaderGLSL gbuffer_shader;
    const char * shaderFileGBuffer = "src/gbuffer.glsl";
    //int status = load_shader_from_file(gbuffer_shader, shaderFileGBuffer, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER | ShaderGLSL::GEOMETRY_SHADER);
    status = load_shader_from_file(gbuffer_shader, shaderFileGBuffer, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::GEOMETRY_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  %s\n", shaderFileGBuffer);
        exit( EXIT_FAILURE );
    }    

    // Compute locations for gbuffer_shader
    GLuint gbuffer_projectionLocation = glGetUniformLocation(gbuffer_shader.program, "Projection");
    GLuint gbuffer_viewLocation = glGetUniformLocation(gbuffer_shader.program, "View");
    GLuint gbuffer_objectLocation = glGetUniformLocation(gbuffer_shader.program, "Object");
    GLuint gbuffer_timeLocation = glGetUniformLocation(gbuffer_shader.program, "Time");
    GLuint gbuffer_diffuseLocation = glGetUniformLocation(gbuffer_shader.program, "Diffuse");
    GLuint gbuffer_specLocation = glGetUniformLocation(gbuffer_shader.program, "Spec");
    GLuint gbuffer_lightPositionLocation = glGetUniformLocation(gbuffer_shader.program, "LightPosition");
    GLuint gbuffer_worldToScreenLocation = glGetUniformLocation(gbuffer_shader.program, "WorldToScreen");
    GLuint gbuffer_nbInstancesLocation = glGetUniformLocation(gbuffer_shader.program, "NbInstances");

    // Load Blit shader
    ShaderGLSL blit_shader;
    const char * shaderFileBlit = "src/blit.glsl";
    //int status = load_shader_from_file(blit_shader, shaderFileBlit, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER | ShaderGLSL::GEOMETRY_SHADER);
    status = load_shader_from_file(blit_shader, shaderFileBlit, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  %s\n", shaderFileBlit);
        exit( EXIT_FAILURE );
    }    
    // Compute locations for blit_shader
    GLuint blit_tex1Location = glGetUniformLocation(blit_shader.program, "Texture1");

    // Load light accumulation shader
    ShaderGLSL lighting_shader;
    const char * shaderFileLighting = "src/light.glsl";
    //int status = load_shader_from_file(lighting_shader, shaderFileLighting, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER | ShaderGLSL::GEOMETRY_SHADER);
    status = load_shader_from_file(lighting_shader, shaderFileLighting, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  %s\n", shaderFileLighting);
        exit( EXIT_FAILURE );
    }    
    // Compute locations for lighting_shader
    GLuint lighting_materialLocation = glGetUniformLocation(lighting_shader.program, "Material");
    GLuint lighting_scatterLocation = glGetUniformLocation(lighting_shader.program, "LightScattering");
    GLuint lighting_normalLocation = glGetUniformLocation(lighting_shader.program, "Normal");
    GLuint lighting_depthLocation = glGetUniformLocation(lighting_shader.program, "Depth");
    GLuint lighting_inverseViewProjectionLocation = glGetUniformLocation(lighting_shader.program, "InverseViewProjection");
    GLuint lighting_worldToScreenLocation = glGetUniformLocation(lighting_shader.program, "WorldToScreen");
    GLuint lighting_cameraPositionLocation = glGetUniformLocation(lighting_shader.program, "CameraPosition");
    GLuint lighting_lightPositionLocation = glGetUniformLocation(lighting_shader.program, "LightPosition");
    GLuint lighting_lightColorLocation = glGetUniformLocation(lighting_shader.program, "LightColor");
    GLuint lighting_lightIntensityLocation = glGetUniformLocation(lighting_shader.program, "LightIntensity");

    // Load gamma shader
    ShaderGLSL gamma_shader;
    const char * shaderFilegamma = "src/gamma.glsl";
    status = load_shader_from_file(gamma_shader, shaderFilegamma, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  %s\n", shaderFilegamma);
        exit( EXIT_FAILURE );
    }    
    // Compute locations for gamma_shader
    GLuint gamma_tex1Location = glGetUniformLocation(gamma_shader.program, "Texture1");
    GLuint gamma_gammaLocation = glGetUniformLocation(gamma_shader.program, "Gamma");

    // Load sobel shader
    ShaderGLSL sobel_shader;
    const char * shaderFilesobel = "src/sobel.glsl";
    status = load_shader_from_file(sobel_shader, shaderFilesobel, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  %s\n", shaderFilesobel);
        exit( EXIT_FAILURE );
    }    
    // Compute locations for sobel_shader
    GLuint sobel_tex1Location = glGetUniformLocation(sobel_shader.program, "Texture1");
    GLuint sobel_sobelLocation = glGetUniformLocation(sobel_shader.program, "SobelCoef");

    // Load blur shader
    ShaderGLSL blur_shader;
    const char * shaderFileblur = "src/blur.glsl";
    status = load_shader_from_file(blur_shader, shaderFileblur, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  %s\n", shaderFileblur);
        exit( EXIT_FAILURE );
    }    
    // Compute locations for blur_shader
    GLuint blur_tex1Location = glGetUniformLocation(blur_shader.program, "Texture1");
    GLuint blur_directionLocation = glGetUniformLocation(blur_shader.program, "Direction");
    GLuint blur_samplesLocation = glGetUniformLocation(blur_shader.program, "SampleCount");

    // Load coc shader
    ShaderGLSL coc_shader;
    const char * shaderFilecoc = "src/coc.glsl";
    status = load_shader_from_file(coc_shader, shaderFilecoc, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  %s\n", shaderFilecoc);
        exit( EXIT_FAILURE );
    }    
    // Compute locations for coc_shader
    GLuint coc_depthLocation = glGetUniformLocation(coc_shader.program, "Depth");
    GLuint coc_screenToViewLocation = glGetUniformLocation(coc_shader.program, "ScreenToView");
    GLuint coc_focusLocation = glGetUniformLocation(coc_shader.program, "Focus");
    GLuint coc_nearLocation = glGetUniformLocation(coc_shader.program, "Near");
    GLuint coc_farLocation = glGetUniformLocation(coc_shader.program, "Far");

    // Load dof shader
    ShaderGLSL dof_shader;
    const char * shaderFiledof = "src/dof.glsl";
    status = load_shader_from_file(dof_shader, shaderFiledof, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  %s\n", shaderFiledof);
        exit( EXIT_FAILURE );
    }    
    // Compute locations for dof_shader
    GLuint dof_colorLocation = glGetUniformLocation(dof_shader.program, "Color");
    GLuint dof_blurLocation = glGetUniformLocation(dof_shader.program, "Blur");
    GLuint dof_cocLocation = glGetUniformLocation(dof_shader.program, "CoC");
    
    
    // Load notexture shader
    ShaderGLSL notex_shader;
    const char * shaderFilenotex = "src/notexture.glsl";
    status = load_shader_from_file(notex_shader, shaderFilenotex, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  %s\n", shaderFilenotex);
        exit( EXIT_FAILURE );
    }else{
		std::cout << "notexture loaded" << std::endl;
	}
	// Compute locations for notex_shader
    GLuint notex_projectionLocation = glGetUniformLocation(notex_shader.program, "Projection");
    GLuint notex_viewLocation = glGetUniformLocation(notex_shader.program, "View");
    GLuint notex_colorLocation = glGetUniformLocation(notex_shader.program, "UColor");
    GLuint notex_positionLocation = glGetUniformLocation(notex_shader.program, "Position");
    GLuint notex_isLightLocation = glGetUniformLocation(notex_shader.program, "isLight");
    GLuint notex_timeLocation = glGetUniformLocation(notex_shader.program, "Time");
    GLuint notex_nbInstancesLocation = glGetUniformLocation(notex_shader.program, "NbInstances");
    
    
    // Load scattering shader
    ShaderGLSL scatter_shader;
    const char * shaderFilescatter = "src/scattering.glsl";
    status = load_shader_from_file(scatter_shader, shaderFilescatter, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  %s\n", shaderFilescatter);
        exit( EXIT_FAILURE );
    }else{
		std::cout << "scattering loaded" << std::endl;
	}
	// Compute locations for scatter_shader
    GLuint scatter_firstPassLocation = glGetUniformLocation(scatter_shader.program, "FirstPass");
    GLuint scatter_lightPositionLocation = glGetUniformLocation(scatter_shader.program, "LightPosition");
    GLuint scatter_worldToScreenLocation = glGetUniformLocation(scatter_shader.program, "WorldToScreen");
    GLuint scatter_exposureLocation = glGetUniformLocation(scatter_shader.program, "exposure");
    GLuint scatter_decayLocation = glGetUniformLocation(scatter_shader.program, "decay");
    GLuint scatter_densityLocation = glGetUniformLocation(scatter_shader.program, "density");
    GLuint scatter_weightLocation = glGetUniformLocation(scatter_shader.program, "weight");
    
    
	float coord = 0.5f;
	
	float lightCoord = 15.0f;

    // Load geometry
    int   cube_triangleCount = 12;
    int   cube_triangleList[] = {0, 1, 2, 2, 1, 3, 4, 5, 6, 6, 5, 7, 8, 9, 10, 10, 9, 11, 12, 13, 14, 14, 13, 15, 16, 17, 18, 19, 17, 20, 21, 22, 23, 24, 25, 26, };
    float cube_uvs[] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f,  1.f, 0.f,  1.f, 1.f,  0.f, 1.f,  1.f, 1.f,  0.f, 0.f, 0.f, 0.f, 1.f, 1.f,  1.f, 0.f,  };
    float cube_vertices[] = {-coord, -coord, coord, coord, -coord, coord, -coord, coord, coord, coord, coord, coord, -coord, coord, coord, coord, coord, coord, -coord, coord, -coord, coord, coord, -coord, -coord, coord, -coord, coord, coord, -coord, -coord, -coord, -coord, coord, -coord, -coord, -coord, -coord, -coord, coord, -coord, -coord, -coord, -coord, coord, coord, -coord, coord, coord, -coord, coord, coord, -coord, -coord, coord, coord, coord, coord, coord, coord, coord, coord, -coord, -coord, -coord, -coord, -coord, -coord, coord, -coord, coord, -coord, -coord, coord, -coord, -coord, -coord, coord, -coord, coord, coord };
    float cube_normals[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, };
    
    // light
    int   light_triangleCount = 12;
    int   light_triangleList[] = {0, 1, 2, 2, 1, 3, 4, 5, 6, 6, 5, 7, 8, 9, 10, 10, 9, 11, 12, 13, 14, 14, 13, 15, 16, 17, 18, 19, 17, 20, 21, 22, 23, 24, 25, 26, };
    float light_uvs[] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f,  1.f, 0.f,  1.f, 1.f,  0.f, 1.f,  1.f, 1.f,  0.f, 0.f, 0.f, 0.f, 1.f, 1.f,  1.f, 0.f,  };
    float light_vertices[] = {-lightCoord, -lightCoord, lightCoord, lightCoord, -lightCoord, lightCoord, -lightCoord, lightCoord, lightCoord, lightCoord, lightCoord, lightCoord, -lightCoord, lightCoord, lightCoord, lightCoord, lightCoord, lightCoord, -lightCoord, lightCoord, -lightCoord, lightCoord, lightCoord, -lightCoord, -lightCoord, lightCoord, -lightCoord, lightCoord, lightCoord, -lightCoord, -lightCoord, -lightCoord, -lightCoord, lightCoord, -lightCoord, -lightCoord, -lightCoord, -lightCoord, -lightCoord, lightCoord, -lightCoord, -lightCoord, -lightCoord, -lightCoord, lightCoord, lightCoord, -lightCoord, lightCoord, lightCoord, -lightCoord, lightCoord, lightCoord, -lightCoord, -lightCoord, lightCoord, lightCoord, lightCoord, lightCoord, lightCoord, lightCoord, lightCoord, lightCoord, -lightCoord, -lightCoord, -lightCoord, -lightCoord, -lightCoord, -lightCoord, lightCoord, -lightCoord, lightCoord, -lightCoord, -lightCoord, lightCoord, -lightCoord, -lightCoord, -lightCoord, lightCoord, -lightCoord, lightCoord, lightCoord };
    float light_normals[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, };
    
    
    int   plane_triangleCount = 2;
    int   plane_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float plane_uvs[] = {0.f, 0.f, 0.f, 10.f, 10.f, 0.f, 10.f, 10.f};
    float plane_vertices[] = {-50.0, -1.0, 50.0, 50.0, -1.0, 50.0, -50.0, -1.0, -50.0, 50.0, -1.0, -50.0};
    float plane_normals[] = {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0};
    int   quad_triangleCount = 2;
    int   quad_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float quad_vertices[] =  {-1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0};

    // Vertex Array Object
    GLuint vao[4];
    glGenVertexArrays(4, vao);

    // Vertex Buffer Objects
    GLuint vbo[14];
    glGenBuffers(14, vbo);
    
    //~ GLuint colVbo[1];
    //~ glGenBuffers(1, colVbo);

    // Cube
    glBindVertexArray(vao[0]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_triangleList), cube_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normals), cube_normals, GL_STATIC_DRAW);
    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_uvs), cube_uvs, GL_STATIC_DRAW);
    
    // light
    glBindVertexArray(vao[3]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[10]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(light_triangleList), light_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[11]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(light_vertices), light_vertices, GL_STATIC_DRAW);
    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[12]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(light_normals), light_normals, GL_STATIC_DRAW);
    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[13]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(light_uvs), light_uvs, GL_STATIC_DRAW);
    
    

    // Plane
    glBindVertexArray(vao[1]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[4]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(plane_triangleList), plane_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_vertices), plane_vertices, GL_STATIC_DRAW);
    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[6]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_normals), plane_normals, GL_STATIC_DRAW);
    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[7]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_uvs), plane_uvs, GL_STATIC_DRAW);

    // Quad
    glBindVertexArray(vao[2]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[8]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_triangleList), quad_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[9]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    // Unbind everything. Potentially illegal on some implementations
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Create gbuffer frame buffer object
    //
    GLuint gbufferFbo;
    GLuint gbufferTextures[3];
    GLuint gbufferDrawBuffers[2];
    glGenTextures(3, gbufferTextures);

    // Create color texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create normal texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create depth texture
    glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    

    // Create Framebuffer Object
    glGenFramebuffers(1, &gbufferFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFbo);

    // Attach textures to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, gbufferTextures[0], 0);
    gbufferDrawBuffers[0] = GL_COLOR_ATTACHMENT0;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1 , GL_TEXTURE_2D, gbufferTextures[1], 0);
    gbufferDrawBuffers[1] = GL_COLOR_ATTACHMENT1;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gbufferTextures[2], 0);
    
    
    // lightScattering fbo
    GLuint lsFbo;
    
    GLuint lsTexture[2];
    glGenTextures(2, lsTexture);
    
    // Create no texture texture !
    glBindTexture(GL_TEXTURE_2D, lsTexture[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Light scattering texture
    glBindTexture(GL_TEXTURE_2D, lsTexture[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    
    glGenFramebuffers(1, &lsFbo);
    
    
    /// bind - attach - render
    
    //fxFramebuffer
    GLuint fxBufferFbo;
    glGenFramebuffers(1, &fxBufferFbo);
    
    GLuint fxBufferTextures[4];
    //~ GLuint fxBufferDrawBuffers[2];
    glGenTextures(4, fxBufferTextures);
    
    // Texture 0
    glBindTexture(GL_TEXTURE_2D, fxBufferTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Texture 1
    glBindTexture(GL_TEXTURE_2D, fxBufferTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Texture 2 - half res
    glBindTexture(GL_TEXTURE_2D, fxBufferTextures[2]);
    //~ glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width/2, height/2, 0, GL_RGBA, GL_FLOAT, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Texture 3 - final texture
    glBindTexture(GL_TEXTURE_2D, fxBufferTextures[3]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Error on building framebuffer\n");
        exit( EXIT_FAILURE );
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Settings
    float blurSamples = 2.0;
    float focusPlane = 5.0;
    float nearPlane = 1.0;
    float farPlane = 50.0;
    float gamma = 1.0;
    float sobelCoef = 1.0;

	// light scattering
	float exposure = 1.0;
	float decay = 0.9;
	float density = 1.0;
	float weight = 1.0;

	float nbInstances = 200;
	
	bool showInterface = true;

    do
    {
        t = glfwGetTime();

        // Mouse states
        int leftButton = glfwGetMouseButton( GLFW_MOUSE_BUTTON_LEFT );
        int rightButton = glfwGetMouseButton( GLFW_MOUSE_BUTTON_RIGHT );
        int middleButton = glfwGetMouseButton( GLFW_MOUSE_BUTTON_MIDDLE );

        if( leftButton == GLFW_PRESS )
            guiStates.turnLock = true;
        else
            guiStates.turnLock = false;

        if( rightButton == GLFW_PRESS )
            guiStates.zoomLock = true;
        else
            guiStates.zoomLock = false;

        if( middleButton == GLFW_PRESS )
            guiStates.panLock = true;
        else
            guiStates.panLock = false;

		int iKey = glfwGetKey(73);
		int jKey = glfwGetKey(74);
		
		if(iKey){
			//~ std::cerr << "I pressed" << std::endl;
			showInterface = true;
		}
		
		if(jKey){
			//~ std::cerr << "J pressed" << std::endl;
			showInterface = false;
		}

        // Camera movements
        int altPressed = glfwGetKey(GLFW_KEY_LSHIFT);
        if (!altPressed && (leftButton == GLFW_PRESS || rightButton == GLFW_PRESS || middleButton == GLFW_PRESS))
        {
            int x; int y;
            glfwGetMousePos(&x, &y);
            guiStates.lockPositionX = x;
            guiStates.lockPositionY = y;
        }
        if (altPressed == GLFW_PRESS)
        {
            int mousex; int mousey;
            glfwGetMousePos(&mousex, &mousey);
            int diffLockPositionX = mousex - guiStates.lockPositionX;
            int diffLockPositionY = mousey - guiStates.lockPositionY;
            if (guiStates.zoomLock)
            {
                float zoomDir = 0.0;
                if (diffLockPositionX > 0)
                    zoomDir = -1.f;
                else if (diffLockPositionX < 0 )
                    zoomDir = 1.f;
                camera_zoom(camera, zoomDir * GUIStates::MOUSE_ZOOM_SPEED);
            }
            else if (guiStates.turnLock)
            {
                camera_turn(camera, diffLockPositionY * GUIStates::MOUSE_TURN_SPEED,
                            diffLockPositionX * GUIStates::MOUSE_TURN_SPEED);

            }
            else if (guiStates.panLock)
            {
                camera_pan(camera, diffLockPositionX * GUIStates::MOUSE_PAN_SPEED,
                            diffLockPositionY * GUIStates::MOUSE_PAN_SPEED);
            }
            guiStates.lockPositionX = mousex;
            guiStates.lockPositionY = mousey;
        }
  
		// light infos
		//~ float lightPosition[3] = { 2.0f, 2.0f, 1.0f};
		float lightPosition[3] = { 0.0f, 20.0f, 0.0f};
		float lightColor[3] = {1.f, 1.f, 1.f};
		float lightIntensity = 200.0f;
		
		float cubeColor[3] = {0.f,0.f,0.f};
		float cubePosition[3] = {0.f,0.f,0.f};
  
        // Get camera matrices
        glm::mat4 projection = glm::perspective(45.0f, widthf / heightf, 0.1f, 100.f); 
        glm::mat4 worldToView = glm::lookAt(camera.eye, camera.o, camera.up);
        glm::mat4 objectToWorld;
        glm::mat4 worldToScreen = projection * worldToView;
        glm::mat4 screenToWorld = glm::transpose(glm::inverse(worldToScreen));
        glm::mat4 screenToView = glm::inverse(projection);



		// no texture rendering
		
		glUseProgram(notex_shader.program);
		glUniformMatrix4fv(notex_projectionLocation, 1, 0, glm::value_ptr(projection));
        glUniformMatrix4fv(notex_viewLocation, 1, 0, glm::value_ptr(worldToView));
        
        glBindFramebuffer(GL_FRAMEBUFFER, lsFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, lsTexture[0], 0);
		//~ glDrawBuffers(1, lsDrawBuffers);
		
		// Viewport 
        glViewport( 0, 0, width, height  );

        // Default states
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);

        // Clear the front buffer
        glClearColor(0.01f, 0.04f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
		
		// Render vaos
		
		// light
		glUniform3fv(notex_colorLocation, 1, lightColor);
		glUniform3fv(notex_positionLocation, 1, lightPosition);
		glUniform1i(notex_isLightLocation, 1);
        glBindVertexArray(vao[3]);
        glDrawElementsInstanced(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0, 1);
        
        // cube / occluders
		glUniform3fv(notex_colorLocation, 1, cubeColor);
		glUniform3fv(notex_positionLocation, 1, cubePosition);
		glUniform1i(notex_isLightLocation, 0);
		glUniform1f(notex_timeLocation, t);
		glUniform1i(notex_nbInstancesLocation, (int)nbInstances);
		
        glBindVertexArray(vao[0]);
        glDrawElementsInstanced(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0, (int)nbInstances);
        

		// end notex
		
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		
		// light scattering
		
		glUseProgram(scatter_shader.program);
		
		glUniform3fv(scatter_lightPositionLocation, 1, lightPosition);
		glUniform1i(scatter_firstPassLocation, 0);
		glUniformMatrix4fv(scatter_worldToScreenLocation, 1, 0, glm::value_ptr(worldToScreen));
		glUniform1f(scatter_exposureLocation, exposure);
		glUniform1f(scatter_decayLocation, decay);
		glUniform1f(scatter_densityLocation, density);
		glUniform1f(scatter_weightLocation, weight);
		
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, lsTexture[1], 0);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, lsTexture[0]);
		
		// Draw quad
		glBindVertexArray(vao[2]);
		glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
		
		// ---------------
        // --- LS BLUR ---
        // ---------------
		// Bind blur shader
		glUseProgram(blur_shader.program);
		// Upload uniforms
        glActiveTexture(GL_TEXTURE0);
		glUniform1i(blur_tex1Location, 0);
		//~ glUniform1i(blur_samplesLocation, (int)blurSamples);
		
		glViewport( 0, 0, width, height);
		
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, lsTexture[0], 0);
		
		glBindTexture(GL_TEXTURE_2D, lsTexture[1]);
		
		// Draw quad
		glBindVertexArray(vao[2]);
		glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
		
		// end light scattering
		
		
        glBindFramebuffer(GL_FRAMEBUFFER, gbufferFbo);
        glDrawBuffers(2, gbufferDrawBuffers);

        // Viewport 
        glViewport( 0, 0, width, height  );

        // Default states
        glEnable(GL_DEPTH_TEST);
		
        // Clear the front buffer
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        

        // Bind gbuffer shader
        glUseProgram(gbuffer_shader.program);
        // Upload uniforms
        glUniformMatrix4fv(gbuffer_projectionLocation, 1, 0, glm::value_ptr(projection));
        glUniformMatrix4fv(gbuffer_viewLocation, 1, 0, glm::value_ptr(worldToView));
        glUniformMatrix4fv(gbuffer_objectLocation, 1, 0, glm::value_ptr(objectToWorld));
        glUniformMatrix4fv(gbuffer_worldToScreenLocation, 1, 0, glm::value_ptr(worldToScreen));
        glUniform3fv(gbuffer_lightPositionLocation, 1, lightPosition);
        glUniform1f(gbuffer_timeLocation, t);
        glUniform1i(gbuffer_nbInstancesLocation, (int)nbInstances);
        glUniform1i(gbuffer_diffuseLocation, 0);
        glUniform1i(gbuffer_specLocation, 1);

        // Bind textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textures[1]);

        // Render vaos
        glBindVertexArray(vao[0]);
        glDrawElementsInstanced(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0, (int)nbInstances);
        
        // Unbind framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport( 0, 0, width, height );

        // Clear the front buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Bind lighting shader
        glUseProgram(lighting_shader.program);
        // Upload uniforms
        glUniform1i(lighting_materialLocation, 0);
        glUniform1i(lighting_normalLocation, 1);
        glUniform1i(lighting_depthLocation, 2);
        glUniform1i(lighting_scatterLocation, 3);
        glUniform3fv(lighting_cameraPositionLocation, 1, glm::value_ptr(camera.eye));
        glUniformMatrix4fv(lighting_inverseViewProjectionLocation, 1, 0, glm::value_ptr(screenToWorld));
        glUniformMatrix4fv(lighting_worldToScreenLocation, 1, 0, glm::value_ptr(worldToScreen));

        // Bind color to unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);        
        // Bind normal to unit 1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);    
        // Bind depth to unit 2
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);        
        // Bind light scattering to unit 3
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, lsTexture[0]);        

        // Blit above the rest
        glDisable(GL_DEPTH_TEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

		
		// bind fx framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, fxBufferFbo);
		// attach texture
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxBufferTextures[0], 0);
		
		// CLEAR
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport( 0, 0, width, height );

        // Clear the front buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		// LIGHTS

		// upload light uniforms
		glUniform3fv(lighting_lightPositionLocation, 1, lightPosition);
		glUniform3fv(lighting_lightColorLocation, 1, lightColor);
		glUniform1f(lighting_lightIntensityLocation, lightIntensity);

		// Draw quad
		glBindVertexArray(vao[2]);
		glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        
        
        // END LIGHTS

        glDisable(GL_BLEND);

		// -----------
        // -- SOBEL --
        // -----------
		// Bind sobel shader
        glUseProgram(sobel_shader.program);
        // Upload uniforms
        glActiveTexture(GL_TEXTURE0);
		glUniform1i(sobel_tex1Location, 0);
		glUniform1f(sobel_sobelLocation, sobelCoef);
		
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxBufferTextures[1], 0);
		
		glBindTexture(GL_TEXTURE_2D, fxBufferTextures[0]);
		
		// Draw quad
		glBindVertexArray(vao[2]);
		glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
		
		/*
		// ------------
        // --- BLUR ---
        // ------------
		// Bind blur shader
		glUseProgram(blur_shader.program);
		// Upload uniforms
        glActiveTexture(GL_TEXTURE0);
		glUniform1i(blur_tex1Location, 0);
		//~ glUniform1i(blur_samplesLocation, (int)blurSamples);
		//~ glUniform1i(blur_samplesLocation, 1); // disable blur
		
		//~ glViewport( 0, 0, width/2, height/2);
		glViewport( 0, 0, width, height);
		
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxBufferTextures[2], 0);
		
		glBindTexture(GL_TEXTURE_2D, fxBufferTextures[1]);
		
		// Draw quad
		glBindVertexArray(vao[2]);
		glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
		*/
		/*
		// -----------
        // --- COC --- // Circle of Confusion
        // -----------
		// Bind coc shader
		// Viewport 
        glViewport( 0, 0, width, height);
		
		glUseProgram(coc_shader.program);
		// Upload uniforms
        glActiveTexture(GL_TEXTURE0);
		glUniform1i(coc_depthLocation, 0);
		glUniform1f(coc_focusLocation, focusPlane);
		glUniform1f(coc_nearLocation, nearPlane);
		glUniform1f(coc_farLocation, farPlane);
		glUniformMatrix4fv(coc_screenToViewLocation, 1, 0, glm::value_ptr(screenToView));
		
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxBufferTextures[0], 0);
		
		glBindTexture(GL_TEXTURE_2D, gbufferTextures[1]);
		
		// Draw quad
		glBindVertexArray(vao[2]);
		glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
		*/
		/*
		// -----------
        // --- DOF --- // Depth of Field
        // -----------
		// Bind dof shader
		glUseProgram(dof_shader.program);
		
		glUniform1i(dof_colorLocation, 0);
		glUniform1i(dof_blurLocation, 1);
		glUniform1i(dof_cocLocation, 2);
		
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxBufferTextures[3], 0);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fxBufferTextures[1]); // color
		
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, fxBufferTextures[2]); // blur
		
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, fxBufferTextures[0]); // coc
		
		// Draw quad
		glBindVertexArray(vao[2]);
		glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
		
		
		
		*/
		
        // -----------
        // -- GAMMA -- sur le framebuffer par défaut
        // -----------
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        //~ /*
        // Viewport 
        glViewport( 0, 0, width, height);

        // Clear the front buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Bind gamma shader
        glUseProgram(gamma_shader.program);
        // Upload uniforms
		glUniform1i(gamma_tex1Location, 0);
		
        glActiveTexture(GL_TEXTURE0);
        //~ glBindTexture(GL_TEXTURE_2D, fxBufferTextures[1]);
        glBindTexture(GL_TEXTURE_2D, fxBufferTextures[1]);
        
		glUniform1f(gamma_gammaLocation, gamma);
		
		//render
		glBindVertexArray(vao[2]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        
        // END FX
        
		if(showInterface){
			// Bind blit shader
			glUseProgram(blit_shader.program);
			// Upload uniforms
			glUniform1i(blit_tex1Location, 0);
			// use only unit 0
			glActiveTexture(GL_TEXTURE0);

			// Viewport 
			glViewport( 0, 0, width/4, height/4  );
			// Bind texture
			//~ glBindTexture(GL_TEXTURE_2D, gbufferTextures[0]);        
			glBindTexture(GL_TEXTURE_2D, lsTexture[0]);        
			// Draw quad
			glBindVertexArray(vao[2]);
			glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
			// Viewport 
			glViewport( width/4, 0, width/4, height/4  );
			// Bind texture
			glBindTexture(GL_TEXTURE_2D, lsTexture[1]);        
			// Draw quad
			glBindVertexArray(vao[2]);
			glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
			// Viewport 
			glViewport( width/4 * 2, 0, width/4, height/4  );
			// Bind texture
			glBindTexture(GL_TEXTURE_2D, gbufferTextures[2]);        
			// Draw quad
			glBindVertexArray(vao[2]);
			glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
			
			// fx
			// Viewport 
			glViewport( width/4 * 3, 0, width/4, height/4  );
			// Bind texture
			glBindTexture(GL_TEXTURE_2D, fxBufferTextures[0]);        
			// Draw quad
			glBindVertexArray(vao[2]);
			glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
		}
        
#if 1
			// Draw UI
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glViewport(0, 0, width, height);
		
		if(showInterface){

			unsigned char mbut = 0;
			int mscroll = 0;
			int mousex; int mousey;
			glfwGetMousePos(&mousex, &mousey);
			mousey = height - mousey;

			if( leftButton == GLFW_PRESS )
				mbut |= IMGUI_MBUT_LEFT;

			imguiBeginFrame(mousex, mousey, mbut, mscroll);
			int logScroll = 0;
			char lineBuffer[512];
			imguiBeginScrollArea("001", width - 210, height - 310, 200, 300, &logScroll);
			sprintf(lineBuffer, "FPS %f", fps);
			imguiLabel(lineBuffer);
			//~ imguiSlider("Lights", &numLights, 0.0, 100.0, 1.0);
			imguiSlider("Gamma", &gamma, 0.1, 6.0, 0.1);
			//~ imguiSlider("Sobel", &sobelCoef, 0.0, 1.0, 0.1);
			//~ imguiSlider("Blur Samples", &blurSamples, 1.0, 100.0, 1.0);
			//~ imguiSlider("Focus plane", &focusPlane, 1.0, 100.0, 1.0);
			//~ imguiSlider("Near plane", &nearPlane, 1.0, 100.0, 1.0);
			//~ imguiSlider("Far plane", &farPlane, 1.0, 100.0, 1.0);
			imguiSlider("Exposure", &exposure, 0.0, 2.0, 0.1);
			imguiSlider("Decay", &decay, 0.0, 2.0, 0.1);
			imguiSlider("Density", &density, 0.0, 0.5, 0.001);
			imguiSlider("Weight", &weight, 0.0, 2.0, 0.1);
			imguiSlider("NbInstances", &nbInstances, 0.0, 1000.0, 100.);
			
			//~ uniform float exposure = 1.0;
			//~ uniform float decay = 0.9;
			//~ uniform float density = 1.0;
			//~ uniform float weight = 1.0;
			imguiEndScrollArea();
			imguiEndFrame();
			imguiRenderGLDraw(width, height);
		}

        glDisable(GL_BLEND);
#endif
        
        // Check for errors
        GLenum err = glGetError();
        if(err != GL_NO_ERROR)
        {
            fprintf(stderr, "OpenGL Error : %s\n", gluErrorString(err));
            
        }

        // Swap buffers
        glfwSwapBuffers();

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey( GLFW_KEY_ESC ) != GLFW_PRESS &&
           glfwGetWindowParam( GLFW_OPENED ) );

    // Clean UI
    imguiRenderGLDestroy();

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    exit( EXIT_SUCCESS );
}




int  compile_and_link_shader(ShaderGLSL & shader, int typeMask, const char * sourceBuffer, int bufferSize)
{
    // Create program object
    shader.program = glCreateProgram();
    
    //Handle Vertex Shader
    GLuint vertexShaderObject ;
    if (typeMask & ShaderGLSL::VERTEX_SHADER)
    {
        // Create shader object for vertex shader
        vertexShaderObject = glCreateShader(GL_VERTEX_SHADER);
        // Add #define VERTEX to buffer
        const char * sc[3] = { "#version 330\n", "#define VERTEX\n", sourceBuffer};
        glShaderSource(vertexShaderObject, 
                       3, 
                       sc,
                       NULL);
        // Compile shader
        glCompileShader(vertexShaderObject);

        // Get error log size and print it eventually
        int logLength;
        glGetShaderiv(vertexShaderObject, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 1)
        {
            char * log = new char[logLength];
            glGetShaderInfoLog(vertexShaderObject, logLength, &logLength, log);
            fprintf(stderr, "Error in compiling vertex shader : %s", log);
            fprintf(stderr, "%s\n%s\n%s", sc[0], sc[1], sc[2]);
            delete[] log;
        }
        // If an error happend quit
        int status;
        glGetShaderiv(vertexShaderObject, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
            return -1;          

        //Attach shader to program
        glAttachShader(shader.program, vertexShaderObject);
    }

    // Handle Geometry shader
    GLuint geometryShaderObject ;
    if (typeMask & ShaderGLSL::GEOMETRY_SHADER)
    {
        // Create shader object for Geometry shader
        geometryShaderObject = glCreateShader(GL_GEOMETRY_SHADER);
        // Add #define Geometry to buffer
        const char * sc[3] = { "#version 330\n", "#define GEOMETRY\n", sourceBuffer};
        glShaderSource(geometryShaderObject, 
                       3, 
                       sc,
                       NULL);
        // Compile shader
        glCompileShader(geometryShaderObject);

        // Get error log size and print it eventually
        int logLength;
        glGetShaderiv(geometryShaderObject, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 1)
        {
            char * log = new char[logLength];
            glGetShaderInfoLog(geometryShaderObject, logLength, &logLength, log);
            fprintf(stderr, "Error in compiling Geometry shader : %s \n", log);
            fprintf(stderr, "%s\n%s\n%s", sc[0], sc[1], sc[2]);
            delete[] log;
        }
        // If an error happend quit
        int status;
        glGetShaderiv(geometryShaderObject, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
            return -1;          

        //Attach shader to program
        glAttachShader(shader.program, geometryShaderObject);
    }


    // Handle Fragment shader
    GLuint fragmentShaderObject ;
    if (typeMask && ShaderGLSL::FRAGMENT_SHADER)
    {
        // Create shader object for fragment shader
        fragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);
        // Add #define fragment to buffer
        const char * sc[3] = { "#version 330\n", "#define FRAGMENT\n", sourceBuffer};
        glShaderSource(fragmentShaderObject, 
                       3, 
                       sc,
                       NULL);
        // Compile shader
        glCompileShader(fragmentShaderObject);

        // Get error log size and print it eventually
        int logLength;
        glGetShaderiv(fragmentShaderObject, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 1)
        {
            char * log = new char[logLength];
            glGetShaderInfoLog(fragmentShaderObject, logLength, &logLength, log);
            fprintf(stderr, "Error in compiling fragment shader : %s \n", log);
            fprintf(stderr, "%s\n%s\n%s", sc[0], sc[1], sc[2]);
            delete[] log;
        }
        // If an error happend quit
        int status;
        glGetShaderiv(fragmentShaderObject, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
            return -1;          

        //Attach shader to program
        glAttachShader(shader.program, fragmentShaderObject);
    }


    // Bind attribute location
    glBindAttribLocation(shader.program,  0,  "VertexPosition");
    glBindAttribLocation(shader.program,  1,  "VertexNormal");
    glBindAttribLocation(shader.program,  2,  "VertexTexCoord");
    //~ glBindAttribLocation(shader.program,  3,  "VertexColor");
    glBindFragDataLocation(shader.program, 0, "Color");
    glBindFragDataLocation(shader.program, 1, "Normal");

    // Link attached shaders
    glLinkProgram(shader.program);

    // Clean
    if (typeMask & ShaderGLSL::VERTEX_SHADER)
    {
        glDeleteShader(vertexShaderObject);
    }
    if (typeMask && ShaderGLSL::GEOMETRY_SHADER)
    {
        glDeleteShader(fragmentShaderObject);
    }
    if (typeMask && ShaderGLSL::FRAGMENT_SHADER)
    {
        glDeleteShader(fragmentShaderObject);
    }

    // Get link error log size and print it eventually
    int logLength;
    glGetProgramiv(shader.program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        char * log = new char[logLength];
        glGetProgramInfoLog(shader.program, logLength, &logLength, log);
        fprintf(stderr, "Error in linking shaders : %s \n", log);
        delete[] log;
    }
    int status;
    glGetProgramiv(shader.program, GL_LINK_STATUS, &status);        
    if (status == GL_FALSE)
        return -1;


    return 0;
}

int  destroy_shader(ShaderGLSL & shader)
{
    glDeleteProgram(shader.program);
    shader.program = 0;
    return 0;
}

int load_shader_from_file(ShaderGLSL & shader, const char * path, int typemask)
{
    int status;
    FILE * shaderFileDesc = fopen( path, "rb" );
    if (!shaderFileDesc)
        return -1;
    fseek ( shaderFileDesc , 0 , SEEK_END );
    long fileSize = ftell ( shaderFileDesc );
    rewind ( shaderFileDesc );
    char * buffer = new char[fileSize + 1];
    fread( buffer, 1, fileSize, shaderFileDesc );
    buffer[fileSize] = '\0';
    status = compile_and_link_shader( shader, typemask, buffer, fileSize );
    delete[] buffer;
    return status;
}


void camera_compute(Camera & c)
{
    c.eye.x = cos(c.theta) * sin(c.phi) * c.radius + c.o.x;   
    c.eye.y = cos(c.phi) * c.radius + c.o.y ;
    c.eye.z = sin(c.theta) * sin(c.phi) * c.radius + c.o.z;   
    c.up = glm::vec3(0.f, c.phi < M_PI ?1.f:-1.f, 0.f);
}

void camera_defaults(Camera & c)
{
    c.phi = 3.14/2.f;
    c.theta = 3.14/2.f;
    c.radius = 10.f;
    camera_compute(c);
}

void camera_zoom(Camera & c, float factor)
{
    c.radius += factor * c.radius ;
    if (c.radius < 0.1)
    {
        c.radius = 10.f;
        c.o = c.eye + glm::normalize(c.o - c.eye) * c.radius;
    }
    camera_compute(c);
}

void camera_turn(Camera & c, float phi, float theta)
{
    c.theta += 1.f * theta;
    c.phi   -= 1.f * phi;
    if (c.phi >= (2 * M_PI) - 0.1 )
        c.phi = 0.00001;
    else if (c.phi <= 0 )
        c.phi = 2 * M_PI - 0.1;
    camera_compute(c);
}

void camera_pan(Camera & c, float x, float y)
{
    glm::vec3 up(0.f, c.phi < M_PI ?1.f:-1.f, 0.f);
    glm::vec3 fwd = glm::normalize(c.o - c.eye);
    glm::vec3 side = glm::normalize(glm::cross(fwd, up));
    c.up = glm::normalize(glm::cross(side, fwd));
    c.o[0] += up[0] * y * c.radius * 2;
    c.o[1] += up[1] * y * c.radius * 2;
    c.o[2] += up[2] * y * c.radius * 2;
    c.o[0] -= side[0] * x * c.radius * 2;
    c.o[1] -= side[1] * x * c.radius * 2;
    c.o[2] -= side[2] * x * c.radius * 2;       
    camera_compute(c);
}
