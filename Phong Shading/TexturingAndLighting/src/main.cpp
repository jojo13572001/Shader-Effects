#include <TextureAndLightingPCH.h>
#include <math.h>
#include <algorithm>

#include <Camera.h>


#define POSITION_ATTRIBUTE 0
#define NORMAL_ATTRIBUTE 2
#define DIFFUSE_ATTRIBUTE 3
#define SPECULAR_ATTRIBUTE 4
#define TEXCOORD0_ATTRIBUTE 8
#define TEXCOORD1_ATTRIBUTE 9
#define TEXCOORD2_ATTRIBUTE 10

#define BUFFER_OFFSET(offset) ((void*)(offset))
#define MEMBER_OFFSET(s,m) ((char*)NULL + (offsetof(s,m)))

// the size will be changed after reshape()
int g_iWindowWidth = 1280;
int g_iWindowHeight = 720;
int g_iWindowHandle = 0;

int g_W, g_A, g_S, g_D, g_Q, g_E;
bool g_bShift = false;
GLuint shaderType = 0;
GLboolean enableEarthNormalMap = false;
GLboolean enableEarthBumpMap = false;

glm::ivec2 g_MousePos;

float g_fSunRotation = 0.0f;
float g_fEarthRotation = 0.0f;
float g_fMoonRotation = 0.0f;

std::clock_t g_PreviousTicks;
std::clock_t g_CurrentTicks;

Camera g_Camera;
glm::vec3 g_InitialCameraPosition;
glm::quat g_InitialCameraRotation;

GLuint g_vaoSphere = 0;
GLuint g_TexturedDiffuseShaderProgram = 0;
GLuint g_SimpleShaderProgram = 0;

// Model, View, Projection matrix uniform variable in shader program.
GLint g_uniformMVP = -1;
GLint g_uniformModelMatrix = -1;
GLint g_uniformEyePosW = -1;

GLint g_uniformColor = -1;

// Light uniform variables.
GLint g_uniformLightPosW = -1;
GLint g_uniformLightColor = -1;
GLint g_uniformAmbient = -1;

// Material properties.
GLint g_uniformMaterialEmissive = -1;
GLint g_uniformMaterialDiffuse = -1;
GLint g_uniformMaterialSpecular = -1;
GLint g_uniformMaterialShininess =-1;
GLint g_uniformShaderType = -1;
GLint g_uniformEnableEarthNormalMap = -1;
GLint g_uniformEnableEarthBumpMap = -1;
std::vector<GLint> g_uniformLuts(2, -1);
GLint g_uniformNormalMap = -1;
GLint g_uniformBumpMap = -1;

GLuint g_EarthTexture = 0;
GLuint g_EarthNormalMap = 0;
GLuint g_EarthBumpMap = 0;
GLuint g_MoonTexture = 0;
std::vector<GLuint> g_LutTextures;

std::vector<std::string> shaderTypes = { "Phong", "Blinn Phong" , "LUT Blinn Phong"};
std::string normalMapHeadline, bumpMapHeadline;
glm::vec4 materialDiffuseEarth(1);
glm::vec4 lightColor(1);
glm::vec4 materialSpecularEarth(2.0f, 2.0f, 2.0f, 1.0f);
GLfloat masterialShininessEarth = 50.0f;


void IdleGL();
void DisplayGL();
void KeyboardGL( unsigned char c, int x, int y );
void KeyboardUpGL( unsigned char c, int x, int y );
void SpecialGL( int key, int x, int y );
void SpecialUpGL( int key, int x, int y );
void MouseGL( int button, int state, int x, int y );
void MotionGL( int x, int y );
void ReshapeGL( int w, int h );

/**
 * Initialize the OpenGL context and create a render window.
 */
void InitGL( int argc, char* argv[] )
{
    std::cout << "Initialize OpenGL..." << std::endl;

    glutInit( &argc, argv );

    glutSetOption( GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS );

    int iScreenWidth = glutGet(GLUT_SCREEN_WIDTH);
    int iScreenHeight = glutGet(GLUT_SCREEN_HEIGHT);

    glutInitDisplayMode(GLUT_RGBA|GLUT_ALPHA|GLUT_DOUBLE|GLUT_DEPTH);

	//comment this because drawStrokeText() not supported
    // Create an OpenGL 3.3 core forward compatible context.
    //glutInitContextVersion( 3, 3 );
    //glutInitContextProfile(GLUT_CORE_PROFILE);
    //glutInitContextFlags( GLUT_FORWARD_COMPATIBLE );

    glutInitWindowPosition( ( iScreenWidth - g_iWindowWidth ) / 2, (iScreenHeight - g_iWindowHeight) / 2 );
    glutInitWindowSize( g_iWindowWidth, g_iWindowHeight );

    g_iWindowHandle = glutCreateWindow("Texturing and Lighting");

    // Register GLUT callbacks.
    glutIdleFunc(IdleGL);
    glutDisplayFunc(DisplayGL);
    glutKeyboardFunc(KeyboardGL);
    glutKeyboardUpFunc(KeyboardUpGL);
    glutSpecialFunc(SpecialGL);
    glutSpecialUpFunc(SpecialUpGL);
    glutMouseFunc(MouseGL);
    glutMotionFunc(MotionGL);
    glutReshapeFunc(ReshapeGL);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f );
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    std::cout << "Initialize OpenGL Success!" << std::endl;
}

void InitGLEW()
{
    glewExperimental = GL_TRUE;
    if ( glewInit() != GLEW_OK )
    {
        std::cerr << "There was a problem initializing GLEW. Exiting..." << std::endl;
        exit(-1);
    }

    // Check for 3.3 support.
    // I've specified that a 3.3 forward-compatible context should be created.
    // so this parameter check should always pass if our context creation passed.
    // If we need access to deprecated features of OpenGL, we should check
    // the state of the GL_ARB_compatibility extension.
    if ( !GLEW_VERSION_3_3 )
    {
        std::cerr << "OpenGL 3.3 required version support not present." << std::endl;
        exit(-1);
    }

#ifdef _WIN32
    if ( WGLEW_EXT_swap_control )
    {
        wglSwapIntervalEXT(0); // Disable vertical sync
    }
#endif
}

// Loads a shader and returns the compiled shader object.
// If the shader source file could not be opened or compiling the 
// shader fails, then this function returns 0.
GLuint LoadShader( GLenum shaderType, const std::string& shaderFile )
{
    std::ifstream ifs;

    // Load the shader.
    ifs.open(shaderFile);

    if ( !ifs )
    {
        std::cerr << "Can not open shader file: \"" << shaderFile << "\"" << std::endl;
        return 0;
    }

    std::string source( std::istreambuf_iterator<char>(ifs), (std::istreambuf_iterator<char>()) );
    ifs.close();

    // Create a shader object.
    GLuint shader = glCreateShader( shaderType );

    // Load the shader source for each shader object.
    const GLchar* sources[] = { source.c_str() };
    glShaderSource( shader, 1, sources, NULL );

    // Compile the shader.
    glCompileShader( shader );

    // Check for errors
    GLint compileStatus;
    glGetShaderiv( shader, GL_COMPILE_STATUS, &compileStatus ); 
    if ( compileStatus != GL_TRUE )
    {
        GLint logLength;
        glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &logLength );
        GLchar* infoLog = new GLchar[logLength];
        glGetShaderInfoLog( shader, logLength, NULL, infoLog );

#ifdef _WIN32
        OutputDebugString(infoLog);
#else
        std::cerr << infoLog << std::endl;
#endif
        delete infoLog;
        return 0;
    }

    return shader;
}

// Create a shader program from a set of compiled shader objects.
GLuint CreateShaderProgram( std::vector<GLuint> shaders )
{
    // Create a shader program.
    GLuint program = glCreateProgram();

    // Attach the appropriate shader objects.
    for( GLuint shader: shaders )
    {
        glAttachShader( program, shader );
    }

    // Link the program
    glLinkProgram(program);

    // Check the link status.
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus );
    if ( linkStatus != GL_TRUE )
    {
        GLint logLength;
        glGetProgramiv( program, GL_INFO_LOG_LENGTH, &logLength );
        GLchar* infoLog = new GLchar[logLength];

        glGetProgramInfoLog( program, logLength, NULL, infoLog );

#ifdef _WIN32
        OutputDebugString(infoLog);
#else
        std::cerr << infoLog << std::endl;
#endif

        delete infoLog;
        return 0;
    }

    return program;
}

GLuint LoadTexture( const std::string& file )
{
    GLuint textureID = SOIL_load_OGL_texture( file.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glBindTexture( GL_TEXTURE_2D, 0 );

    return textureID;
}

std::vector<GLuint> LoadLookupTable(int width, int height,
					   GLfloat specShineness,
					   const glm::vec4& lightColor,
					   const glm::vec4& materialDiffuseEarth,
					   const glm::vec4& materialSpecularEarth)
{
	std::vector<GLuint> lutTextures(2);
	std::vector<GLubyte> dataDiffuse(width * height * 4);
	std::vector<GLubyte> dataSpecular(width * 1 * 4);
	GLuint uniformLut = 0;
	
	for (int y = 0, idx = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x, idx += 4) {
			float vx = x / float(width);
			float vy = y / float(height);
			float nl = vx;
			float nh = vy;
			float spec = powf(nh, specShineness);
			glm::vec4 resultDiffuse = nl*lightColor*materialDiffuseEarth*255.0f;
			dataDiffuse[idx + 0] = round(std::min(255.0f, resultDiffuse.r));
			dataDiffuse[idx + 1] = round(std::min(255.0f, resultDiffuse.g));
			dataDiffuse[idx + 2] = round(std::min(255.0f, resultDiffuse.b));
			dataDiffuse[idx + 3] = round(std::min(255.0f, spec*255.0f));
		}
	}

	for (int x = 0, idx = 0; x < width; ++x, idx += 4) {
		float vx = x / float(width);
		float spec = powf(vx, specShineness);
		glm::vec4 resultSpecular = spec*lightColor*materialSpecularEarth*255.0f;
		dataSpecular[idx + 0] = round(std::min(255.0f, resultSpecular.r));
		dataSpecular[idx + 1] = round(std::min(255.0f, resultSpecular.g));
		dataSpecular[idx + 2] = round(std::min(255.0f, resultSpecular.b));
		dataSpecular[idx + 3] = round(std::min(255.0f, resultSpecular.a));
	}

	auto normalize = [=](std::vector<GLfloat>& data) {
		for (int channel = 0; channel < 3; channel++) {
			GLfloat max = data[channel], min = data[channel];
			int idx = 0;
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x, idx += 4) {
					if (max < data[idx + channel]) {
						max = data[idx + channel];
					}
					if (min > data[idx + channel]) {
						min = data[idx + channel];
					}
				}
			}
			GLfloat interval = max - min;
			idx = 0;
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x, idx += 4) {
					data[idx + channel] = (data[idx + channel] - min) / interval * 255.0f;
				}
			}
		}
	};

	glGenTextures(2, &lutTextures[0]);
	glBindTexture(GL_TEXTURE_2D, lutTextures[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &dataDiffuse[0]);

	glBindTexture(GL_TEXTURE_2D, lutTextures[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &dataSpecular[0]);
	glBindTexture(GL_TEXTURE_2D, 0);
	return lutTextures;
}

GLuint SolidSphere( float radius, int slices, int stacks )
{
    using namespace glm;
    using namespace std;

    const float pi = 3.1415926535897932384626433832795f;
    const float _2pi = 2.0f * pi;

    vector<vec3> positions;
    vector<vec3> normals;
    vector<vec2> textureCoords;

    for( int i = 0; i <= stacks; ++i )
    {
        // V texture coordinate.
        float V = i / (float)stacks;
        float phi = V * pi;

        for ( int j = 0; j <= slices; ++j )
        {
            // U texture coordinate.
            float U = j / (float)slices;
            float theta = U * _2pi;

            float X = cos(theta) * sin(phi);
            float Y = cos(phi);
            float Z = sin(theta) * sin(phi);

            positions.push_back( vec3( X, Y, Z) * radius );
            normals.push_back( vec3(X, Y, Z) );
            textureCoords.push_back( vec2(U, V) );
        }
    }

    // Now generate the index buffer
    vector<GLuint> indicies;

    for( int i = 0; i < slices * stacks + slices; ++i )
    {
        indicies.push_back( i );
        indicies.push_back( i + slices + 1  );
        indicies.push_back( i + slices );

        indicies.push_back( i + slices + 1  );
        indicies.push_back( i );
        indicies.push_back( i + 1 );
    }

    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );

    GLuint vbos[4];
    glGenBuffers( 4, vbos );

    glBindBuffer( GL_ARRAY_BUFFER, vbos[0] );
    glBufferData( GL_ARRAY_BUFFER, positions.size() * sizeof(vec3), positions.data(), GL_STATIC_DRAW );
    glVertexAttribPointer( POSITION_ATTRIBUTE, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
    glEnableVertexAttribArray( POSITION_ATTRIBUTE );

    glBindBuffer( GL_ARRAY_BUFFER, vbos[1] );
    glBufferData( GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), normals.data(), GL_STATIC_DRAW );
    glVertexAttribPointer( NORMAL_ATTRIBUTE, 3, GL_FLOAT, GL_TRUE, 0, BUFFER_OFFSET(0) );
    glEnableVertexAttribArray( NORMAL_ATTRIBUTE );

    glBindBuffer( GL_ARRAY_BUFFER, vbos[2] );
    glBufferData( GL_ARRAY_BUFFER, textureCoords.size() * sizeof(vec2), textureCoords.data(), GL_STATIC_DRAW );
    glVertexAttribPointer( TEXCOORD0_ATTRIBUTE, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
    glEnableVertexAttribArray( TEXCOORD0_ATTRIBUTE );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vbos[3] );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, indicies.size() * sizeof(GLuint), indicies.data(), GL_STATIC_DRAW );

    glBindVertexArray( 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

    return vao;
}

int main( int argc, char* argv[] )
{
    g_PreviousTicks = std::clock();
    g_A = g_W = g_S = g_D = g_Q = g_E = 0;

    g_InitialCameraPosition = glm::vec3( 0, 0, 60 );
    g_Camera.SetPosition( g_InitialCameraPosition );
    g_Camera.SetRotation( g_InitialCameraRotation );

    InitGL(argc, argv);
    InitGLEW();

    g_EarthTexture = LoadTexture( "../data/Textures/earth2k.jpg" );
	g_EarthNormalMap = LoadTexture("../data/Textures/normal8k.dds");
	g_EarthBumpMap = LoadTexture("../data/Textures/bump1k.jpg");
    g_MoonTexture = LoadTexture( "../data/Textures/moon.dds" );
	
	//creat lookup table texture
	int width = 1024, height = 1024;
	g_LutTextures = LoadLookupTable(width, height, masterialShininessEarth, lightColor, materialDiffuseEarth, materialSpecularEarth);

    GLuint vertexShader = LoadShader( GL_VERTEX_SHADER, "../data/shaders/simpleShader.vert" );
    GLuint fragmentShader = LoadShader( GL_FRAGMENT_SHADER, "../data/shaders/simpleShader.frag" );

    std::vector<GLuint> shaders;
    shaders.push_back(vertexShader);
    shaders.push_back(fragmentShader);

    g_SimpleShaderProgram = CreateShaderProgram( shaders );
    assert( g_SimpleShaderProgram );

    // Set the color uniform variable in the simple shader program to white.
    g_uniformColor = glGetUniformLocation( g_SimpleShaderProgram, "color" );

    vertexShader = LoadShader( GL_VERTEX_SHADER, "../data/shaders/texturedDiffuse.vert" );
    fragmentShader = LoadShader( GL_FRAGMENT_SHADER, "../data/shaders/texturedDiffuse.frag" );

    shaders.clear();

    shaders.push_back(vertexShader);
    shaders.push_back(fragmentShader);
    g_TexturedDiffuseShaderProgram = CreateShaderProgram( shaders );
    assert( g_TexturedDiffuseShaderProgram );

    g_uniformMVP = glGetUniformLocation( g_TexturedDiffuseShaderProgram, "ModelViewProjectionMatrix" );
    g_uniformModelMatrix = glGetUniformLocation( g_TexturedDiffuseShaderProgram, "ModelMatrix" );
    g_uniformEyePosW = glGetUniformLocation( g_TexturedDiffuseShaderProgram, "EyePosW" );

    // Light properties.
    g_uniformLightPosW = glGetUniformLocation( g_TexturedDiffuseShaderProgram, "LightPosW" );
    g_uniformLightColor = glGetUniformLocation( g_TexturedDiffuseShaderProgram, "LightColor" );

    // Global ambient.
    g_uniformAmbient = glGetUniformLocation( g_TexturedDiffuseShaderProgram, "Ambient" );

    // Material properties.
    g_uniformMaterialEmissive = glGetUniformLocation( g_TexturedDiffuseShaderProgram, "MaterialEmissive" );
    g_uniformMaterialDiffuse = glGetUniformLocation( g_TexturedDiffuseShaderProgram, "MaterialDiffuse" );
    g_uniformMaterialSpecular = glGetUniformLocation( g_TexturedDiffuseShaderProgram, "MaterialSpecular" );
    g_uniformMaterialShininess = glGetUniformLocation( g_TexturedDiffuseShaderProgram, "MaterialShininess" );
	g_uniformShaderType = glGetUniformLocation( g_TexturedDiffuseShaderProgram, "shaderType" );
	g_uniformLuts[0] = glGetUniformLocation(g_TexturedDiffuseShaderProgram, "lutDiffuseSampler");
	g_uniformLuts[1] = glGetUniformLocation(g_TexturedDiffuseShaderProgram, "lutSpecularSampler");
	g_uniformNormalMap = glGetUniformLocation(g_TexturedDiffuseShaderProgram, "normalMapSampler");
	g_uniformBumpMap = glGetUniformLocation(g_TexturedDiffuseShaderProgram, "bumpMapSampler");
	g_uniformEnableEarthNormalMap = glGetUniformLocation(g_TexturedDiffuseShaderProgram, "enableEarthNormalMap");
	g_uniformEnableEarthBumpMap = glGetUniformLocation(g_TexturedDiffuseShaderProgram, "enableEarthBumpMap");

    glutMainLoop();
}

void ReshapeGL( int w, int h )
{
    if ( h == 0 )
    {
        h = 1;
    }

    g_iWindowWidth = w;
    g_iWindowHeight = h;

    g_Camera.SetViewport( 0, 0, w, h );
    g_Camera.SetProjectionRH( 30.0f, w/(float)h, 0.1f, 200.0f );

	gluOrtho2D(0, w,0, h);

    glutPostRedisplay();
}

void DisplayGL()
{
    int slices = 32;
    int stacks = 32;
	//for fps calculate
	static int currentTicks = 0, previousTicks = 0;
	static float fDeltaTime = 0.0f;
	static int frameCount = 0;
	static std::string fps = "0 fps";

    int numIndicies = ( slices * stacks + slices ) * 6;
    if ( g_vaoSphere == 0 )
    {
        g_vaoSphere = SolidSphere( 1, slices, stacks );
    }

    //const glm::vec4 white(1);
    const glm::vec4 black(0);
    const glm::vec4 ambient( 0.1f, 0.1f, 0.1f, 1.0f );

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // Draw the sun using a simple shader.
    glBindVertexArray(g_vaoSphere);

    glUseProgram( g_SimpleShaderProgram );
    glm::mat4 modelMatrix = glm::rotate( glm::radians(g_fSunRotation), glm::vec3(0,-1,0) ) * glm::translate(glm::vec3(90,0,-50));
    glm::mat4 mvp = g_Camera.GetProjectionMatrix() * g_Camera.GetViewMatrix() * modelMatrix;
    GLuint uniformMVP = glGetUniformLocation( g_SimpleShaderProgram, "MVP" );
    glUniformMatrix4fv( uniformMVP, 1, GL_FALSE, glm::value_ptr(mvp) );
    glUniform4fv(g_uniformColor, 1, glm::value_ptr(lightColor) );

    glDrawElements( GL_TRIANGLES, numIndicies, GL_UNSIGNED_INT, BUFFER_OFFSET(0) );
	
	//Activate and bind textures to opengl for all shader usage
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_EarthTexture);

	// Load the LUT
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, g_LutTextures[0]);

	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, g_LutTextures[1]);

	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_2D, g_EarthNormalMap);

	glActiveTexture(GL_TEXTURE0 + 4);
	glBindTexture(GL_TEXTURE_2D, g_EarthBumpMap);

	//start using earth shader
    glUseProgram( g_TexturedDiffuseShaderProgram );

	glUniform1i(g_uniformLuts[0], 1); // location = 9 <- texture unit 1
	glUniform1i(g_uniformLuts[1], 2);
	glUniform1i(g_uniformNormalMap, 3);
	glUniform1i(g_uniformBumpMap, 4);

    // Set the light position to the position of the Sun.
    glUniform4fv( g_uniformLightPosW, 1, glm::value_ptr(modelMatrix[3]) );
    glUniform4fv( g_uniformLightColor, 1, glm::value_ptr(lightColor) );
    glUniform4fv( g_uniformAmbient, 1, glm::value_ptr(ambient) );

	//switch between all shading types
	glUniform1i(g_uniformShaderType, shaderType);
	glUniform1i(g_uniformEnableEarthNormalMap, enableEarthNormalMap);
	glUniform1i(g_uniformEnableEarthBumpMap, enableEarthBumpMap);

    
	// Draw the Earth
    modelMatrix = glm::rotate( glm::radians(g_fEarthRotation), glm::vec3(0,1,0) ) * glm::scale(glm::vec3(12.756f) );
    glm::vec4 eyePosW = glm::vec4( g_Camera.GetPosition(), 1 );

    mvp = g_Camera.GetProjectionMatrix() * g_Camera.GetViewMatrix() * modelMatrix;

    glUniformMatrix4fv( g_uniformMVP, 1, GL_FALSE, glm::value_ptr(mvp) );
    glUniformMatrix4fv( g_uniformModelMatrix, 1, GL_FALSE, glm::value_ptr(modelMatrix) );
    glUniform4fv( g_uniformEyePosW, 1, glm::value_ptr( eyePosW ) );

    // Earth Diffuse, Emissive Material properties.
    glUniform4fv( g_uniformMaterialEmissive, 1, glm::value_ptr(black) );
    glUniform4fv( g_uniformMaterialDiffuse, 1, glm::value_ptr(materialDiffuseEarth) );

	// Earth Specular Color RGBA
    glUniform4fv( g_uniformMaterialSpecular, 1, glm::value_ptr(materialSpecularEarth));
    glUniform1f( g_uniformMaterialShininess, masterialShininessEarth );

    glDrawElements( GL_TRIANGLES, numIndicies, GL_UNSIGNED_INT, BUFFER_OFFSET(0) );
	/*
    // Draw the moon.
    glBindTexture( GL_TEXTURE_2D, g_MoonTexture );

    modelMatrix =  glm::rotate( glm::radians(g_fSunRotation), glm::vec3(0,1,0) ) * glm::translate(glm::vec3(60, 0, 0) ) * glm::scale(glm::vec3(3.476f));
    mvp = g_Camera.GetProjectionMatrix() * g_Camera.GetViewMatrix() * modelMatrix;

    glUniformMatrix4fv( g_uniformMVP, 1, GL_FALSE, glm::value_ptr(mvp) );
    glUniformMatrix4fv( g_uniformModelMatrix, 1, GL_FALSE, glm::value_ptr(modelMatrix) );

    glUniform4fv( g_uniformMaterialEmissive, 1, glm::value_ptr(black) );
    glUniform4fv( g_uniformMaterialDiffuse, 1, glm::value_ptr(white) );
    glUniform4fv( g_uniformMaterialSpecular, 1, glm::value_ptr(white) );
    glUniform1f( g_uniformMaterialShininess, 5.0f );

    glDrawElements( GL_TRIANGLES, numIndicies, GL_UNSIGNED_INT, BUFFER_OFFSET(0) );
	*/
    glBindVertexArray(0);
    glUseProgram(0);
    glBindTexture( GL_TEXTURE_2D, 0 );

	frameCount++;
	currentTicks = std::clock();
	float deltaTicks = (float)(currentTicks - previousTicks);
	previousTicks = currentTicks;
	fDeltaTime += deltaTicks / (float)CLOCKS_PER_SEC;
	
	if (fDeltaTime > 2.0f) {
		float fpsRate = frameCount/fDeltaTime;
		fDeltaTime = 0.0f;
		frameCount = 0;
		fps = std::to_string(fpsRate) + " fps";
	}

	drawStrokeText(const_cast<char*>(fps.c_str()), 0, g_iWindowHeight*0.9, 0);
	drawStrokeText(const_cast<char*>((shaderTypes[shaderType]+normalMapHeadline+ bumpMapHeadline).c_str()), 0, g_iWindowHeight*0.1, 0);
		
    glutSwapBuffers();
}

void IdleGL()
{
    g_CurrentTicks = std::clock();
    float deltaTicks = (float)( g_CurrentTicks - g_PreviousTicks );
    g_PreviousTicks = g_CurrentTicks;

    float fDeltaTime = deltaTicks / (float)CLOCKS_PER_SEC;

    float speed = 1.0f;

    if ( g_bShift )
    {
		speed = 2.0f;
    }

    g_Camera.Translate( glm::vec3( g_D - g_A, g_Q - g_E, g_S - g_W) * speed * fDeltaTime );

    // Rate of rotation in (degrees) per second
    const float fRotationRate1 = 30.0f*speed;
    const float fRotationRate2 = 12.5f;
    const float fRotationRate3 = 90.0f;

    g_fEarthRotation += fRotationRate1 * fDeltaTime;
    g_fEarthRotation = fmod(g_fEarthRotation, 360.0f);

    g_fMoonRotation = fRotationRate2;
    //g_fMoonRotation = fmod(g_fMoonRotation, 360.0f);

    g_fSunRotation = fRotationRate3;
    //g_fSunRotation = fmod(g_fSunRotation, 360.0f);

    glutPostRedisplay();
}

void KeyboardGL( unsigned char c, int x, int y )
{
    switch ( c )
    {
    case 'w':
    case 'W':
        g_W = 1;
        break;
    case 'a':
    case 'A':
        g_A = 1;
        break;
    case 's':
    case 'S':
        g_S = 1;
        break;
    case 'd':
    case 'D':
        g_D = 1;
        break;
    case 'q':
    case 'Q':
        g_Q = 1;
        break;
    case 'e':
    case 'E':
        g_E = 1;
        break;
    case 'r':
    case 'R':
        g_Camera.SetPosition( g_InitialCameraPosition );
        g_Camera.SetRotation( g_InitialCameraRotation );
        g_fEarthRotation = 0.0f;
        g_fSunRotation = 0.0f;
        g_fMoonRotation = 0.0f;
        break;
	case 'T':
	case 't':
		++shaderType %= shaderTypes.size();
		normalMapHeadline = (enableEarthNormalMap && shaderType!=2) ? " (Normal Map)" : "";
		break;
	case 'N' :
	case 'n' :
		enableEarthNormalMap = !enableEarthNormalMap;
		normalMapHeadline = (enableEarthNormalMap && shaderType!=2) ? " (Normal Map)" : "";
		break;
	case 'B':
	case 'b':
		enableEarthBumpMap = !enableEarthBumpMap;
		bumpMapHeadline = (enableEarthBumpMap) ? " (bump Map)" : "";
		break;
    case 27:
        glutLeaveMainLoop();
        break;
    }
}

void KeyboardUpGL( unsigned char c, int x, int y )
{
    switch ( c )
    {
    case 'w':
    case 'W':
        g_W = 0;
        break;
    case 'a':
    case 'A':
        g_A = 0;
        break;
    case 's':
    case 'S':
        g_S = 0;
        break;
    case 'd':
    case 'D':
        g_D = 0;
        break;
    case 'q':
    case 'Q':
        g_Q = 0;
        break;
    case 'e':
    case 'E':
        g_E = 0;
        break;

    default:
        break;
    }
}

void SpecialGL( int key, int x, int y )
{
    switch( key )
    {
    case GLUT_KEY_SHIFT_L:
    case GLUT_KEY_SHIFT_R:
        {
            g_bShift = true;
        }
        break;
    }
}

void SpecialUpGL( int key, int x, int y )
{
    switch( key )
    {
    case GLUT_KEY_SHIFT_L:
    case GLUT_KEY_SHIFT_R:
        {
            g_bShift = false;
        }
        break;
    }
}

void MouseGL( int button, int state, int x, int y )
{
    g_MousePos = glm::ivec2(x,y);
}

void MotionGL( int x, int y )
{
    glm::ivec2 mousePos = glm::ivec2( x, y );
    glm::vec2 delta = glm::vec2( mousePos - g_MousePos );
    g_MousePos = mousePos;

    std::cout << "dX: " << delta.x << " dy: " << delta.y << std::endl;

    glm::quat rotX = glm::angleAxis<float>( glm::radians(delta.y) * 0.5f, glm::vec3(1, 0, 0) );
    glm::quat rotY = glm::angleAxis<float>( glm::radians(delta.x) * 0.5f, glm::vec3(0, 1, 0) );

    //g_Camera.Rotate( rotX * rotY );
//    g_Rotation = ( rotX * rotY ) * g_Rotation;

}
