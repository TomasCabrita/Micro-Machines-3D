//
// AVT 2026: Texturing with Phong Shading and Text rendered with TrueType library
// The text rendering was based on https://dev.to/shreyaspranav/how-to-render-truetype-fonts-in-opengl-using-stbtruetypeh-1p5k
// You can also learn an alternative with FreeType text: https://learnopengl.com/In-Practice/Text-Rendering
// This demo was built for learning purposes only.
// Some code could be severely optimised, but I tried to
// keep as simple and clear as possible.
//
// The code comes with no warranties, use it at your own risk.
// You may use it, or parts of it, wherever you want.
// 
// Author: João Madeiras Pereira
//

#include <math.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// include GLEW to access OpenGL 3.3 functions
#include <GL/glew.h>

// GLUT is the toolkit to interface with the OS
#include <GL/freeglut.h>

#include <IL/il.h>

#include <algorithm>

#include "renderer.h"
#include "shader.h"
#include "mathUtility.h"
#include "model.h"
#include "texture.h"

using namespace std;

#define CAPTION "AVTM 2026 Welcome Demo"
int WindowHandle = 0;
int WinX = 640, WinY = 480;

unsigned int FrameCount = 0;

//File with the font
const string fontPathFile = "fonts/arial.ttf";

//Object of class gmu (Graphics Math Utility) to manage math and matrix operations
gmu mu;

//Object of class renderer to manage the rendering of meshes and ttf-based bitmap text
Renderer renderer;
	
// Camera Position
float camX, camY, camZ;

// Camera Spherical Coordinates
float alpha = 57.0f, _beta = 18.0f;
float r = 45.0f;

//Cameras (new)
//1 = satellite orthographic
//2 = satellite perspective
//3 = car following perspective
int CameraMode = 1;

//Aspect ratio da janela
float aspectRatio = 640.0f / 480.0f;


// Mouse Tracking Variables
int startX, startY, tracking = 0;

// Frame counting and FPS computation
#define FPS 60
long myTime,timebase = 0,frame = 0;
char s[32];

//float lightPos[4] = {4.0f, 5.0f, 2.0f, 1.0f};
float lightPos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

//Spotlight
bool spotlight_mode = false;
float coneDir[4] = { 0.0f, -0.0f, -1.0f, 0.0f };

bool fontLoaded = false;


const int CAR_NORMAL_MESH = 9;   // rosa
const int CAR_METAL_MESH = 10;  // metal
const int CAR_GLASS_MESH = 11;  // vidro cinza-azulado
const int CAR_WHEEL_MESH = 12;  // roda
const int CAR_LIGHT_MESH = 13;  // farois
const int CAR_PLATE_MESH = 14;  // matricula

struct Car {
	// Posição no mundo
	float x = 60.0f;
	float y = 0.0f;
	float z = -15.0f;

	// Orientação
	float angle = 0.0f;

	// Dimensões gerais
	float width = 4.5f;
	float height = 3.6f;
	float depth = 7.0f;

	// Movimento
	float speed = 0.0f;
	float acceleration = 0.0f;
	float maxSpeed = 10.0f;
};

Car carBarbie;

/// ::::::::::::::::::::::::::::::::::::::::::::::::AUXILIARY FUNCIONS:::::::::::::::::::::::::::::::::::::::::::::::::://///

// Auxiliary function to draw a mesh object with the given position, scale, and mesh ID
void drawObject(int meshID,
	float posX, float posY, float posZ,
	float scaleX, float scaleY, float scaleZ,
	float rotAngle = 0.0f,
	int texMode = 1) {

	mu.pushMatrix(gmu::MODEL);
	mu.translate(gmu::MODEL, posX, posY, posZ); // Translate the cube to the desired position
	mu.rotate(gmu::MODEL, rotAngle, 1.0f, 0.0f, 0.0f); // Rotate the cube around the X-axis
	mu.scale(gmu::MODEL, scaleX, scaleY, scaleZ); // Scale the cube to the desired size
	mu.translate(gmu::MODEL, -0.5f, -0.5f, -0.5f); // Center the cube at the origin

	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
	mu.computeNormalMatrix3x3();

	dataMesh data;
	data.meshID = meshID;
	data.texMode = texMode; // 0:no texturing; 1:modulate diffuse color with texel color; 2:diffuse color is replaced by texel color; 3: multitexturing
	data.vm = mu.get(gmu::VIEW_MODEL),
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
	data.normal = mu.getNormalMatrix();
	renderer.renderMesh(data);
	mu.popMatrix(gmu::MODEL);
}

void drawCenteredObject(
	int meshID,
	float posX, float posY, float posZ,
	float scaleX, float scaleY, float scaleZ,
	float rotX = 0.0f,
	float rotY = 0.0f,
	float rotZ = 0.0f,
	int texMode = 0)
{
	mu.pushMatrix(gmu::MODEL);

	mu.translate(gmu::MODEL, posX, posY, posZ);

	mu.rotate(gmu::MODEL, rotX, 1.0f, 0.0f, 0.0f);
	mu.rotate(gmu::MODEL, rotY, 0.0f, 1.0f, 0.0f);
	mu.rotate(gmu::MODEL, rotZ, 0.0f, 0.0f, 1.0f);

	mu.scale(gmu::MODEL, scaleX, scaleY, scaleZ);

	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
	mu.computeNormalMatrix3x3();

	dataMesh data;
	data.meshID = meshID;
	data.texMode = texMode;
	data.vm = mu.get(gmu::VIEW_MODEL);
	data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
	data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
	data.normal = mu.getNormalMatrix();

	renderer.renderMesh(data);

	mu.popMatrix(gmu::MODEL);
}

void drawCar(const Car& car)
{
	mu.pushMatrix(gmu::MODEL);
	mu.translate(gmu::MODEL, car.x, car.y, car.z);
	mu.rotate(gmu::MODEL, car.angle, 0.0f, 1.0f, 0.0f);


	// 1 - para choques (trás)
	drawObject(CAR_METAL_MESH,			// material
		0.0f, 0.90f, -1.5375f,			// posição
		car.width -0.5f, 3.675f, 1.10f, // tamanho
		-90.0f,							// rotação
		0);


	// 2 - bagagem (separacao entre vidro e choques) (Trás)
	drawObject(CAR_NORMAL_MESH,
		0.0f, 1.071f, -1.304f,
		car.width, 1.682f, 0.4f,
		-36.03f, 0);

	// 3 - Vidro (trás)
	drawObject(CAR_GLASS_MESH,
		0.0f, 2.1f, -0.554f,
		car.width -0.4f, 1.6f, 1.196f,
		-58.67f,0);

	// 4 - Parte de trás (onde está o vidro) (trás)
	drawObject(CAR_NORMAL_MESH,
		0.0f, 1.712f, -0.554f,
		car.width, 1.924f, 2.196f,
		-58.67f, 0);

	// 5 - teto
	drawObject(CAR_NORMAL_MESH,
		0.0f, 2.088f, 0.0f,
		car.width, 2.2f, 1.70f,
		0.0f, 0);


	// 4.5 - Parte da frente (onde está o vidro) (frente)
	drawObject(CAR_NORMAL_MESH,
		0.0f, 1.712f, 0.554f,
		car.width, 1.924f, 2.196f,
		58.67f,0);

	// 3.5 - Vidro (frente)
	drawObject(CAR_GLASS_MESH,
		0.0f, 2.1f, 0.554f,
		car.width - 0.4f, 1.6f, 1.196f,
		58.67f, 0);

	// 2.5 - bagagem (separacao entre vidro e choques) (frente)
	drawObject(CAR_NORMAL_MESH,
		0.0f, 1.071f, 1.304f,
		car.width, 1.682f, 0.4f,
		36.03f, 0);

	// 1.5 - para choques (frente)
	drawObject(CAR_METAL_MESH,
		0.0f, 0.5f, 1.5375f,
		car.width - 0.5f, 3.675f, 0.2f, 
		90.0f, 0);

	// matricula
	drawObject(CAR_PLATE_MESH,
		0.0f, 1.0f, 1.5375f,
		car.width - 3.0f, 3.675f, 0.5f, 
		90.0f, 0);

	// Farol esquerdo
	drawCenteredObject(CAR_LIGHT_MESH,
	-1.7f, 1.18f, 3.3f,
	0.38f, 0.38f, 0.12f,
	0.0f, 0.0f, 0.0f,
	0);

	// Farol direito
	drawCenteredObject(CAR_LIGHT_MESH,
	1.7f, 1.18f, 3.3f,
	0.38f, 0.38f, 0.12f,
	0.0f, 0.0f, 0.0f,
	0);

	// Base principal (baixo)
	drawObject(CAR_NORMAL_MESH,
		0.0f, 0.963f, 0.0f,
		car.width, 1.375f, 6.70f,
		0.0f,0);

	// Rodas
	float wheelX = car.width * 0.5f;  // lados do carro
	float wheelY = 0.65f;             // altura da roda
	float wheelZ = 2.15f;             // frente/trás

	// Roda esquerda trás
	drawCenteredObject(CAR_WHEEL_MESH,
		-wheelX, wheelY, -wheelZ,
		1.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 90.0f,
		0);

	// Roda direita trás
	drawCenteredObject(CAR_WHEEL_MESH,
		wheelX, wheelY, -wheelZ,
		1.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 90.0f,
		0);

	// Roda esquerda frente
	drawCenteredObject(CAR_WHEEL_MESH,
		-wheelX, wheelY, wheelZ,
		1.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 90.0f,
		0);

	// Roda direita frente
	drawCenteredObject(CAR_WHEEL_MESH,
		wheelX, wheelY, wheelZ,
		1.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 90.0f,
		0);

	mu.popMatrix(gmu::MODEL);
}


/// ::::::::::::::::::::::::::::::::::::::::::::::::CALLBACK FUNCIONS:::::::::::::::::::::::::::::::::::::::::::::::::://///

void timer(int value)
{
	std::ostringstream oss;
	oss << CAPTION << ": " << FrameCount << " FPS @ (" << WinX << "x" << WinY << ")";
	std::string s = oss.str();
	glutSetWindow(WindowHandle);
	glutSetWindowTitle(s.c_str());
    FrameCount = 0;
    glutTimerFunc(1000, timer, 0);
}

void refresh(int value)
{
	//PUT YOUR CODE HERE
	glutPostRedisplay();
	glutTimerFunc(1000 / FPS, refresh, 0);
}

// ------------------------------------------------------------
//
// Reshape Callback Function
//
// Atualiza o viewpoint quando a janela muda de tamanho

void changeSize(int w, int h) {

	/* (Antigo - lightDemo)
	float ratio;
	// Prevent a divide by zero, when window is too short
	if(h == 0)
		h = 1;
	// set the viewport to be the entire window
	glViewport(0, 0, w, h);
	// set the projection matrix
	ratio = (1.0f * w) / h;
	mu.loadIdentity(gmu::PROJECTION);
	mu.perspective(53.13f, ratio, 0.1f, 1000.0f);
	*/

	// Prevent a divide by zero, when window is too short
	if (h == 0)
		h = 1;

	WinX = w;
	WinY = h;

	// set the viewport to be the entire window
	glViewport(0, 0, w, h);
	//guarda o aspecto ratio para as cameras
	aspectRatio = (float)w / (float)h;

}

//Decide qual projecao usar - depende das cameras (adaptavel ao tamanho da mesa)
void setupCamera(float tableWidth, float tableDepth, float tablePosY) {
	// Reconstroi
	mu.loadIdentity(gmu::VIEW);
	mu.loadIdentity(gmu::PROJECTION);

	// margem da camara com mais 10% de espaco do que a mesa ocupa
	float margin = 1.10f;

	//tamanho da camara dependendo da mesa
	float halfView = std::max(tableDepth * 0.5f, (tableWidth * 0.5f) / aspectRatio);
	halfView *= margin;

//Camera 1 - Fixed top camera + orthographic
	if (CameraMode == 1) {
		mu.ortho(-halfView * aspectRatio, halfView * aspectRatio, -halfView, halfView, 0.1f, 1000.0f);

		mu.lookAt(
			0.0f, 200.0f, 0.0f,		//posicao
			0.0f, tablePosY, 0.0f,	// olha para o centro da mesa
			0.0f, 0.0f, -1.0f		// cima da mesa (e não em baixo)
		);
	}

//Camera 2 - Fixed top camera + prespective
	else if (CameraMode == 2) {


		mu.perspective(53.13f, aspectRatio, 0.1f, 1000.0f);

		mu.lookAt(
			0.0f, tablePosY + (1.4f * halfView), (1.8f * halfView),	//posicao
			0.0f, tablePosY, 0.0f,	//para onde olha
			0.0f, 0.0f, -1.0f	// cima da imagem
		);
	}

// Camera 3 - Moving camera + prespective (aceita rato)
	else if (CameraMode == 3) {
		mu.perspective(53.13f, aspectRatio, 0.1f, 1000.0f);

		// A orientação da camara acompanha a orientação do carro
		float carAngleRad = carBarbie.angle * 3.14f / 180.0f;

		float offsetX = camX * cos(carAngleRad) + camZ * sin(carAngleRad);
		float offsetZ = -camX * sin(carAngleRad) + camZ * cos(carAngleRad);

		// Posição final da camara no mundo
		float cameraX = carBarbie.x + offsetX;
		float cameraY = carBarbie.y + 1.5f + camY;
		float cameraZ = carBarbie.z + offsetZ;

		// Camara olha para o carro
		mu.lookAt(cameraX, cameraY, cameraZ,
			carBarbie.x, carBarbie.y + 1.5f, carBarbie.z,
			0.0f, 1.0f, 0.0f);

	}

}


// ------------------------------------------------------------
//
// Render stufff
//

void renderSim(void) {

	FrameCount++;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderer.activateRenderMeshesShaderProg(); // use the required GLSL program to draw the meshes with illumination

	//Associar os Texture Units aos Objects Texture
	//stone.tga loaded in TU0; checker.tga loaded in TU1;  lightwood.tga loaded in TU2
	renderer.setTexUnit(0, 0);
	renderer.setTexUnit(1, 1);
	renderer.setTexUnit(2, 2);


	/* antigo (lightDemo)
	// load identity matrices
	mu.loadIdentity(gmu::VIEW);
	mu.loadIdentity(gmu::MODEL);
	// set the camera using a function similar to gluLookAt
	mu.lookAt(camX, camY, camZ, 0, 0, 0, 0, 1, 0);


	*/

	//send the light position in eye coordinates
	renderer.setLightPos(lightPos); //efeito capacete do mineiro, ou seja lighPos foi definido em eye coord 

	//float lposAux[4];
	//mu.multMatrixPoint(gmu::VIEW, lightPos, lposAux);   //lightPos definido em World Coord so is converted to eye space
	//renderer.setLightPos(lposAux);

	//Spotlight settings
	renderer.setSpotLightMode(spotlight_mode);
	renderer.setSpotParam(coneDir, 0.93);

	// Geometry parameters to scale and translate the objects in the scene
	float tableWidth = 150.0f, tableHeight = 1.0f, tableDepth = 150.0f;
	float tablePosY = -1.0f;

	float roadWidth = 10.0f, roadHeight = 0.1f;
	float roadPosY = tablePosY * 0.5f;

	float marginWidth = 1.0f, marginHeight = 1.0f;
	float marginPosY = roadPosY + 0.3f;

	carBarbie.y = roadPosY + roadHeight * 0.5f + 0.2f;

	//Reset da MODEL e inicia a camara
	mu.loadIdentity(gmu::MODEL);
	setupCamera(tableWidth, tableDepth, tablePosY);




	// Draw the table - myMeshes[0] contains the cube object
	drawObject(0, 0.0f,  tablePosY, 0.0f, tableWidth, tableHeight, tableDepth);

	// Draw the road - myMeshes[1] contains the cube object
	drawObject(1,  60.0f, roadPosY,        -5.0f,  roadWidth, roadHeight, 70.0f);	      // 1: Start road
	drawObject(1,  30.0f, roadPosY,         35.0f, 70.0f,     roadHeight, roadWidth);	  // 2: Horizontal road
	drawObject(1,  0.0f,  roadPosY + 7.5f,  14.0f, roadWidth, roadHeight, 40.0f, 25.0f);  // 3: Inclined vertical road
	drawObject(1,  0.0f,  roadPosY,        -25.0f, roadWidth, roadHeight, 50.0f);		  // 4: Vertical road
	drawObject(1, -30.0f, roadPosY,        -55.0f, 70.0f,     roadHeight, roadWidth);	  // 5: Horizontal road
	drawObject(1, -60.0f, roadPosY + 2.5f, -40.5f, roadWidth, roadHeight, 20.0f, -15.0f); // 6: Inclined vertical road
	drawObject(1, -60.0f, roadPosY + 5.0f, -26.0f, roadWidth, roadHeight, 10.0f);	      // 7: Vertical road
	drawObject(1, -60.0f, roadPosY + 7.5f, -11.5f, roadWidth, roadHeight, 20.0f, -15.0f); // 8: Inclined vertical road
	drawObject(1, -60.0f, roadPosY + 10.0f, 3.0f,  roadWidth, roadHeight, 10.0f);	      // 9: Vertical road
	drawObject(1, -60.0f, roadPosY + 7.5f,  17.5f, roadWidth, roadHeight, 20.0f, 15.0f);  // 10: Inclined vertical road
	drawObject(1, -60.0f, roadPosY + 5.0f,  32.0f, roadWidth, roadHeight, 10.0f);	      // 11: Vertical road
	drawObject(1, -60.0f, roadPosY + 2.5f,  46.5f, roadWidth, roadHeight, 20.0f, 15.0f);  // 12: Inclined vertical road
	drawObject(1, -40.0f, roadPosY,		    60.0f, 50.0f,     roadHeight, roadWidth);	  // 13: Horizontal road
	drawObject(1, -20.0f, roadPosY,         40.0f, roadWidth, roadHeight, 30.0f);		  // 14: Vertical road
	drawObject(1,  10.0f, roadPosY,			20.0f, 70.0f,     roadHeight, roadWidth);	  // 15: Horizontal road
	drawObject(1,  40.0f, roadPosY,        -12.5f, roadWidth, roadHeight, 55.0f);		  // 16: Vertical road
	drawObject(1,  50.0f, roadPosY,        -45.0f, 30.0f,     roadHeight, roadWidth);	  // 17: Horizontal road

	// Draw the margins - myMeshes[2] contains the cube object
	drawObject(2,  55.0f, marginPosY,        -5.0f,  marginWidth, marginHeight, 70.0f);         // 1.1: Left start road margin
	drawObject(2,  65.0f, marginPosY,        -5.0f,  marginWidth, marginHeight, 90.0f);         // 1.2: Right start road margin
	drawObject(2,  30.0f, marginPosY,         30.0f, 50.0f,       marginHeight, marginWidth);   // 2.1: Front horizontal road margin
	drawObject(2,  30.0f, marginPosY,         40.0f, 70.0f,       marginHeight, marginWidth);   // 2.2: Back horizontal road maring
	drawObject(2, -5.0f,  marginPosY,         35.0f, marginWidth, marginHeight, 10.0f);         // 2.3: Left horizontal road margin
	drawObject(2, -5.0f,  marginPosY + 7.5f,  14.0f, marginWidth, marginHeight, 40.0f, 25.0f);  // 3.1: Left inclined vertical road margin
	drawObject(2,  5.0f,  marginPosY + 7.5f,  14.0f, marginWidth, marginHeight, 40.0f, 25.0f);  // 3.2: Right inclined vertical road margin
	drawObject(2,  0.0f,  marginPosY,         0.0f,  10.0f,       marginHeight, marginWidth);   // 4.1: Back vertical road margin
	drawObject(2, -5.0f,  marginPosY,        -25.0f, marginWidth, marginHeight, 50.0f);		    // 4.2: Left vertical road margin
	drawObject(2,  5.0f,  marginPosY,        -30.0f, marginWidth, marginHeight, 60.0f);		    // 4.3: Right vertical road margin
	drawObject(2, -30.0f, marginPosY,        -60.0f, 70.0f,       marginHeight, marginWidth);   // 5.1: Front horizontal road margin
	drawObject(2, -30.0f, marginPosY,        -50.0f, 50.0f,       marginHeight, marginWidth);   // 5.2: Back horizontal road margin
	drawObject(2, -65.0f, marginPosY,        -55.0f, marginWidth, marginHeight, 10.0f);         // 5.3: Left horizontal road margin
	drawObject(2, -65.0f, marginPosY + 2.5f, -40.5f, marginWidth, marginHeight, 20.0f, -15.0f); // 6.1: Left inclined vertical road margin
	drawObject(2, -55.0f, marginPosY + 2.5f, -40.5f, marginWidth, marginHeight, 20.0f, -15.0f); // 6.2: Right inclined vertical road margin
	drawObject(2, -65.0f, marginPosY + 5.0f, -26.0f, marginWidth, marginHeight, 10.0f);	        // 7.1: Left vertical road margin
	drawObject(2, -55.0f, marginPosY + 5.0f, -26.0f, marginWidth, marginHeight, 10.0f);	        // 7.2: Right vertical road margin
	drawObject(2, -65.0f, marginPosY + 7.5f, -11.5f, marginWidth, marginHeight, 20.0f, -15.0f); // 8.1: Left inclined vertical road margin
	drawObject(2, -55.0f, marginPosY + 7.5f, -11.5f, marginWidth, marginHeight, 20.0f, -15.0f); // 8.2: Right inclined vertical road margin
	drawObject(2, -65.0f, marginPosY + 10.0f, 3.0f,  marginWidth, marginHeight, 10.0f);	        // 9.1: Left vertical road margin
	drawObject(2, -55.0f, marginPosY + 10.0f, 3.0f,  marginWidth, marginHeight, 10.0f);	        // 9.2: Right vertical road margin
	drawObject(2, -65.0f, marginPosY + 7.5f,  17.5f, marginWidth, marginHeight, 20.0f, 15.0f);  // 10.1: Left inclined vertical road margin
	drawObject(2, -55.0f, marginPosY + 7.5f,  17.5f, marginWidth, marginHeight, 20.0f, 15.0f);  // 10.2: Right inclined vertical road margin
	drawObject(2, -65.0f, marginPosY + 5.0f,  32.0f, marginWidth, marginHeight, 10.0f);	        // 11.1: Left vertical road margin
	drawObject(2, -55.0f, marginPosY + 5.0f,  32.0f, marginWidth, marginHeight, 10.0f);	        // 11.2: Right vertical road margin
	drawObject(2, -65.0f, marginPosY + 2.5f,  46.5f, marginWidth, marginHeight, 20.0f, 15.0f);  // 12.1: Left inclined vertical road margin
	drawObject(2, -55.0f, marginPosY + 2.5f,  46.5f, marginWidth, marginHeight, 20.0f, 15.0f);  // 12.2: Right inclined vertical road margin
	drawObject(2, -40.0f, marginPosY,         55.0f, 30.0f,       marginHeight, marginWidth);	// 13.1: Front horizontal road margin
	drawObject(2, -40.0f, marginPosY,         65.0f, 50.0f,       marginHeight, marginWidth);	// 13.2: Back horizontal road margin
	drawObject(2, -65.0f, marginPosY,         60.0f, marginWidth, marginHeight, 10.0f);	        // 13.3: Left horizontal road margin
	drawObject(2, -25.0f, marginPosY,         35.0f, marginWidth, marginHeight, 40.0f);		    // 14.1: Left vertical road margin
	drawObject(2, -15.0f, marginPosY,         45.0f, marginWidth, marginHeight, 40.0f);		    // 14.2: Right vertical road margin
	drawObject(2,  5.0f,  marginPosY,         15.0f, 60.0f,       marginHeight, marginWidth);	// 15.1: Front horizontal road margin
	drawObject(2,  15.0f, marginPosY,         25.0f, 60.0f,       marginHeight, marginWidth);	// 15.2: Back horizontal road margin
	drawObject(2,  35.0f, marginPosY,        -17.5f, marginWidth, marginHeight, 65.0f);		    // 16.1: Left vertical road margin
	drawObject(2,  45.0f, marginPosY,        -7.5f,  marginWidth, marginHeight, 65.0f);		    // 16.2: Right vertical road margin
	drawObject(2,  50.0f, marginPosY,        -50.0f, 30.0f,       marginHeight, marginWidth);	// 17.1: Front horizontal road margin
	drawObject(2,  50.0f, marginPosY,        -40.0f, 10.0f,       marginHeight, marginWidth);	// 17.2: Back horizontal road margin

	// Draw the start flag and start line
	drawObject(1, 55.0f, roadPosY + 7.5f,  -5.0f, 1.0f,      15.0f, 1.0f); // 1: Left flag pole
	drawObject(1, 65.0f, roadPosY + 7.5f,  -5.0f, 1.0f,      15.0f, 1.0f); // 2: Right flag pole
	drawObject(2, 60.0f, roadPosY + 10.0f, -5.0f, roadWidth, 3.0f,  1.0f); // 3: Flag
	drawObject(2, 60.0f, roadPosY + 0.1f,  -5.0f, roadWidth, 0.1f,  1.0f); // 4: Start line

	drawCar(carBarbie);
	glutSwapBuffers();
}

// ------------------------------------------------------------
//
// Events from the Keyboard
//

void processKeys(unsigned char key, int xx, int yy)
{
	switch(key) {
		//Cameras (1,2,3)
		case '1':
			CameraMode = 1;
			printf("Camera 1: Fixed orthogonal camera - satellite top view\n");
			break;

		case '2':
			CameraMode = 2;
			printf("Camera 2: Fixed prespective camera - satellite top view\n");
			break;

		case '3':
			CameraMode = 3;
			
			alpha = 180.0f;
			_beta = 18.0f;
			r = 10.0f;
			camX = r * sin(alpha * 3.14f / 180.0f) * cos(_beta * 3.14f / 180.0f);
			camY = r * sin(_beta * 3.14f / 180.0f);
			camZ = r * cos(alpha * 3.14f / 180.0f) * cos(_beta * 3.14f / 180.0f);

			printf("Camera 3: Moving prespective camera - car following\n");
			break;

		case 27:
			glutLeaveMainLoop();
			break;

		case 'c': 
			printf("Camera Spherical Coordinates (%f, %f, %f)\n", alpha, _beta, r);
			break;

		case 'l':   //toggle spotlight mode
			if (!spotlight_mode) {
				spotlight_mode = true;
				printf("Point light disabled. Spot light enabled\n");
			}
			else {
				spotlight_mode = false;
				printf("Spot light disabled. Point light enabled\n");
			}
			break;

		case 'r':    //reset
			alpha = 57.0f; _beta = 18.0f;  // Camera Spherical Coordinates
			r = 45.0f;
			camX = r * sin(alpha * 3.14f / 180.0f) * cos(_beta * 3.14f / 180.0f);
			camZ = r * cos(alpha * 3.14f / 180.0f) * cos(_beta * 3.14f / 180.0f);
			camY = r * sin(_beta * 3.14f / 180.0f);
			break;

		case 'm': glEnable(GL_MULTISAMPLE); break;
		case 'n': glDisable(GL_MULTISAMPLE); break;
	}
}


// ------------------------------------------------------------
//
// Mouse Events
//

void processMouseButtons(int button, int state, int xx, int yy)
{
	// start tracking the mouse
	if (state == GLUT_DOWN)  {
		startX = xx;
		startY = yy;
		if (button == GLUT_LEFT_BUTTON)
			tracking = 1;
		else if (button == GLUT_RIGHT_BUTTON)
			tracking = 2;
	}

	//stop tracking the mouse
	else if (state == GLUT_UP) {
		if (tracking == 1) {
			alpha -= (xx - startX);
			_beta += (yy - startY);
		}
		else if (tracking == 2) {
			r += (yy - startY) * 0.01f;
			if (r < 0.1f)
				r = 0.1f;
		}
		tracking = 0;
	}
}

// Track mouse motion while buttons are pressed

void processMouseMotion(int xx, int yy)
{

	int deltaX, deltaY;
	float alphaAux, betaAux;
	float rAux;

	deltaX =  - xx + startX;
	deltaY =    yy - startY;

	// left mouse button: move camera
	if (tracking == 1) {


		alphaAux = alpha + deltaX;
		betaAux = _beta + deltaY;

		if (betaAux > 85.0f)
			betaAux = 85.0f;
		else if (betaAux < -85.0f)
			betaAux = -85.0f;
		rAux = r;
	}
	// right mouse button: zoom
	else if (tracking == 2) {

		alphaAux = alpha;
		betaAux = _beta;
		rAux = r + (deltaY * 0.01f);
		if (rAux < 0.1f)
			rAux = 0.1f;
	}

	camX = rAux * sin(alphaAux * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
	camZ = rAux * cos(alphaAux * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
	camY = rAux *   						       sin(betaAux * 3.14f / 180.0f);

//  uncomment this if not using an idle or refresh func
//	glutPostRedisplay();
}


void mouseWheel(int wheel, int direction, int x, int y) {

	r += direction * 0.1f;
	if (r < 0.1f)
		r = 0.1f;

	camX = r * sin(alpha * 3.14f / 180.0f) * cos(_beta * 3.14f / 180.0f);
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(_beta * 3.14f / 180.0f);
	camY = r *   						     sin(_beta * 3.14f / 180.0f);

//  uncomment this if not using an idle or refresh func
//	glutPostRedisplay();
}


//
// Scene building with basic geometry
//
// 0: table cube
// 1: road cube
// 2: margin cube
// 3: cube
// 4: pawn
// 5: sphere
// 6: cylinder
// 7: cone
// 8: torus
// 9: car body cube (red)

void buildScene()
{
	//Texture Object definition
	renderer.TexObjArray.texture2D_Loader("assets/stone.tga");
	renderer.TexObjArray.texture2D_Loader("assets/checker.png");
	renderer.TexObjArray.texture2D_Loader("assets/lightwood.tga");

	//Scene geometry with triangle meshes

	MyMesh amesh;

	float amb[] = { 0.2f, 0.15f, 0.1f, 1.0f };
	float diff[] = { 0.8f, 0.6f, 0.4f, 1.0f };
	float spec[] = { 0.8f, 0.8f, 0.8f, 1.0f };

	float amb1[] = { 0.3f, 0.0f, 0.0f, 1.0f };
	float diff1[] = { 0.8f, 0.1f, 0.1f, 1.0f };
	float spec1[] = { 0.3f, 0.3f, 0.3f, 1.0f };

	float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float shininess = 100.0f;
	int texcount = 0;

	// create geometry and VAO of the table
	float ambTable[] = { 0.2f, 0.15f, 0.1f, 1.0f };
	float diffTable[] = { 0.8f, 0.6f, 0.4f, 1.0f };
	float specTable[] = { 0.8f, 0.8f, 0.8f, 1.0f };

	amesh = createCube();
	memcpy(amesh.mat.ambient, ambTable, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffTable, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specTable, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO of the road
	float ambRoad[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	float diffRoad[] = { 0.4f, 0.4f, 0.4f, 1.0f };
	float specRoad[] = { 0.8f, 0.8f, 0.8f, 1.0f };

	amesh = createCube();
	memcpy(amesh.mat.ambient, ambRoad, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffRoad, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specRoad, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO of the road margins
	float ambMargin[] = { 0.2f, 0.0f, 0.15f, 1.0f };
	float diffMargin[] = { 0.8f, 0.0f, 0.6f, 1.0f };
	float specMargin[] = { 0.8f, 0.8f, 0.8f, 1.0f };

	amesh = createCube();
	memcpy(amesh.mat.ambient, ambMargin, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffMargin, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specMargin, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO of the cube
	amesh = createCube();
	memcpy(amesh.mat.ambient, amb1, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff1, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec1, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO of the pawn
	amesh = createPawn();
	memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO of the sphere
	amesh = createSphere(1.0f, 20);
	memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO of the cylinder
	amesh = createCylinder(1.5f, 0.5f, 20);
	memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO of the cone
	amesh = createCone(2.5f, 1.2f, 20);
	memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO of the torus
	amesh = createTorus(0.5f, 1.5f, 20, 20);
	memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO for the car body (9) - cubo rosa
	float ambCar[] = { 0.25f, 0.03f, 0.15f, 1.0f };
	float diffCar[] = { 0.95f, 0.20f, 0.60f, 1.0f };
	float specCar[] = { 0.90f, 0.80f, 0.90f, 1.0f };
	amesh = createCube();
	memcpy(amesh.mat.ambient, ambCar, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffCar, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specCar, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = 100.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO for the metal car parts (10) - cubo metal
	float ambBumper[] = {0.04f, 0.04f, 0.05f, 1.0f};
	float diffBumper[] = {0.16f, 0.17f, 0.19f, 1.0f};
	float specBumper[] = {0.85f, 0.88f, 0.95f, 1.0f};
	amesh = createCube();
	memcpy(amesh.mat.ambient, ambBumper, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffBumper, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specBumper, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = 180.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO for the window car (11) - cubo janela
	float ambGlass[] = {0.08f, 0.12f, 0.16f, 1.0f};
	float diffGlass[] = {0.35f, 0.65f, 0.85f, 1.0f};
	float specGlass[] = {0.80f, 0.90f, 1.00f, 1.0f};
	amesh = createCube();
	memcpy(amesh.mat.ambient, ambGlass, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffGlass, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specGlass, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = 220.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO for the car's wheels (12) - donut roda
	float ambWheel[] = {0.02f, 0.02f, 0.02f, 1.0f};
	float diffWheel[] = {0.06f, 0.06f, 0.07f, 1.0f};
	float specWheel[] = {0.20f, 0.20f, 0.22f, 1.0f};
	amesh = createTorus(0.38f, 0.90f, 20, 20);
	memcpy(amesh.mat.ambient, ambWheel, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffWheel, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specWheel, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = 40.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO for the car's headlights (13) - farois carro
	float ambHeadlight[] = {0.35f, 0.35f, 0.30f, 1.0f};
	float diffHeadlight[] = {1.00f, 0.95f, 0.80f, 1.0f};
	float specHeadlight[] = {1.00f, 1.00f, 1.00f, 1.0f};
	float emissiveHeadlight[] = {0.25f, 0.23f, 0.18f, 1.0f};
	amesh = createSphere(1.0f, 20);
	memcpy(amesh.mat.ambient, ambHeadlight, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffHeadlight, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specHeadlight, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissiveHeadlight, 4 * sizeof(float));
	amesh.mat.shininess = 200.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO for the car's plate (14) - matricula
	float ambPlate[] = {0.30f, 0.30f, 0.30f, 1.0f};
	float diffPlate[] = {0.90f, 0.90f, 0.90f, 1.0f};
	float specPlate[] = {0.25f, 0.25f, 0.25f, 1.0f};
	amesh = createCube();
	memcpy(amesh.mat.ambient, ambPlate, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffPlate, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specPlate, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = 40.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);


	//The truetypeInit creates a texture object in TexObjArray for storing the fontAtlasTexture
	
	fontLoaded = renderer.truetypeInit(fontPathFile);
	if (!fontLoaded)
		cerr << "Fonts not loaded\n";
	else 
		cerr << "Fonts loaded\n";

	printf("\nNumber of Texture Objects is %d\n\n", renderer.TexObjArray.getNumTextureObjects());

	// set the camera position based on its spherical coordinates
	camX = r * sin(alpha * 3.14f / 180.0f) * cos(_beta * 3.14f / 180.0f);
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(_beta * 3.14f / 180.0f);
	camY = r * sin(_beta * 3.14f / 180.0f);
}

// ------------------------------------------------------------
//
// Main function
//

int main(int argc, char **argv) {

//  GLUT initialization
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH|GLUT_DOUBLE|GLUT_RGBA|GLUT_MULTISAMPLE);

	glutInitContextVersion (4, 3);
	glutInitContextProfile (GLUT_CORE_PROFILE );
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);

	glutInitWindowPosition(100,100);
	glutInitWindowSize(WinX, WinY);
	WindowHandle = glutCreateWindow(CAPTION);

//  Callback Registration
	glutDisplayFunc(renderSim);
	glutReshapeFunc(changeSize);

	glutTimerFunc(0, timer, 0);
	glutIdleFunc(renderSim);  // Use it for maximum performance
	//glutTimerFunc(0, refresh, 0);    //use it to to get 60 FPS whatever

//	Mouse and Keyboard Callbacks
	glutKeyboardFunc(processKeys);
	glutMouseFunc(processMouseButtons);
	glutMotionFunc(processMouseMotion);
	glutMouseWheelFunc ( mouseWheel ) ;
	

//	return from main loop
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

//	Init GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	// some GL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	printf ("Vendor: %s\n", glGetString (GL_VENDOR));
	printf ("Renderer: %s\n", glGetString (GL_RENDERER));
	printf ("Version: %s\n", glGetString (GL_VERSION));
	printf ("GLSL: %s\n", glGetString (GL_SHADING_LANGUAGE_VERSION));

	/* Initialization of DevIL */
	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
	{
		printf("wrong DevIL version \n");
		exit(0);
	}
	ilInit();

	buildScene();

	if(!renderer.setRenderMeshesShaderProg("shaders/mesh.vert", "shaders/mesh.frag") || 
		!renderer.setRenderTextShaderProg("shaders/ttf.vert", "shaders/ttf.frag"))
	return(1);

	//  GLUT main loop
	glutMainLoop();

	return(0);
}

