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

//Cameras
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

// Directional light (Day/Night mode)
bool dayMode = true;
float lightDir[4] = { -0.5f, -1.0f, -0.5f, 0.0f };

// Point lights (Candles)
bool candleMode = true;
float candlePos[6][4] = {
	{  55.0f, 15.0f,  35.0f, 1.0f }, // Candle 1
	{  -5.5f, 15.0f,  52.5f, 1.0f }, // Candle 2
	{ -48.5f, 15.0f,  55.0f, 1.0f }, // Candle 3
	{ -37.5f, 15.0f, -55.0f, 1.0f }, // Candle 4
	{  21.5f, 15.0f, -20.0f, 1.0f }, // Candle 5
	{  59.5f, 15.0f, -45.0f, 1.0f }  // Candle 6
};

// Spotlights (Headlights)
bool headlightMode = false;
float spotCosCutOff = 35.0f;
float spotEx = 8.0f;

//float lightPos[4] = {4.0f, 5.0f, 2.0f, 1.0f};
float lightPos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

//Spotlight
bool spotlight_mode = false;
float coneDir[4] = { 0.0f, -0.0f, -1.0f, 0.0f };

bool fontLoaded = false;

// Constants for mesh IDs
const int TABLE_MESH = 0; // table
const int ROAD_MESH = 1; // road
const int MARGIN_MESH = 2; // margin
const int CAR_NORMAL_MESH = 3; // pink
const int CAR_METAL_MESH = 4; // metal
const int CAR_GLASS_MESH = 5; // gray-blue glass
const int CAR_WHEEL_MESH = 6; // tires
const int CAR_LIGHT_MESH = 7; // headlights
const int CAR_PLATE_MESH = 8; // plate
const int CANDLE_BASE_MESH = 9; // candle base
const int CANDLE_WICK_MESH = 10; // candle wick
const int BUTTER_YELLOW_MESH = 11; // butter
const int BUTTER_BEIGE_MESH = 12; // beige part of the butter package
const int BUTTER_BLUE_MESH = 13; // blue part of the butter package

struct Car {
	// Posição no mundo
	float x = 70.0f;
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

struct Butter {
	float x;
	float z;
};

Butter butters[] = {
	//{ 70.0f, -5.0f },
	{ 70.0f, 25.0f },    // estrada 1
	{ 50.0f, 50.0f },    // estrada 2
	{ 10.0f, 30.0f },    // estrada 3
	{-20.0f, 50.0f },    // estrada 4
	{-45.0f, 70.0f },    // estrada 5
	{-70.0f, 30.0f },    // estrada 6
	{-70.0f, -35.0f },   // estrada 6
	{-40.0f, -70.0f },   // estrada 7
	{ 0.0f, -35.0f },    // estrada 8
	{ 20.0f, 0.0f },     // estrada 9
	{ 40.0f, -30.0f },   // estrada 10
	{ 60.0f, -60.0f }    // estrada 11
};

const int NUM_BUTTERS = 12;

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
	float rotX = 0.0f, float rotY = 0.0f, float rotZ = 0.0f,
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

void drawButter(const Butter& butter)
{
	mu.pushMatrix(gmu::MODEL);

	mu.translate(gmu::MODEL, butter.x, 0.0f, butter.z);

	float height = 1.2f;
	float depth = 2.0f;

	// Yellow
	drawObject(
		BUTTER_YELLOW_MESH,
		-2.1f, 0.5f, 0.0f,
		0.8f, height, depth,
		0.0f, 0
	);

	// Small beige stripe
	drawObject(
		BUTTER_BEIGE_MESH,
		-1.45f, 0.5f, 0.0f,
		0.5f, height, depth,
		0.0f, 0
	);

	// Blue stripe
	drawObject(
		BUTTER_BLUE_MESH,
		-0.75f, 0.5f, 0.0f,
		0.9f, height, depth,
		0.0f, 0
	);

	// Large beige part
	drawObject(
		BUTTER_BEIGE_MESH,
		0.95f, 0.5f, 0.0f,
		2.5f, height, depth,
		0.0f, 0
	);



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
	renderer.setTexUnit(3, 3);

	//// load identity matrices
	//mu.loadIdentity(gmu::VIEW);
	//mu.loadIdentity(gmu::MODEL);
	//// set the camera using a function similar to gluLookAt
	//mu.lookAt(camX, camY, camZ, 0, 0, 0, 0, 1, 0);

	// Geometry parameters to scale and translate the objects in the scene
	float tableWidth = 175.0f, tableHeight = 1.0f, tableDepth = 175.0f;
	float tablePosY = -1.0f;

	float roadWidth = 20.0f, roadHeight = 0.1f;
	float roadPosY = tablePosY * 0.5f;

	float marginWidth = 1.0f, marginHeight = 1.0f;
	float marginPosY = roadPosY + 0.3f;

	float candleBasePosY = roadPosY + 6.0f;
	float candleWickPosY = roadPosY + 12.0f + 1.5f;

	carBarbie.y = roadPosY + roadHeight * 0.5f + 0.2f;

	// Reset the model matrix and set up the camera based on the table dimensions
	mu.loadIdentity(gmu::MODEL);
	setupCamera(tableWidth, tableDepth, tablePosY);

	// Set directional light mode (day/night) and transform the light direction to eye space
	float dirEye[3];
	float dirAux[4];
	mu.multMatrixPoint(gmu::VIEW, lightDir, dirAux);
	dirEye[0] = dirAux[0]; dirEye[1] = dirAux[1]; dirEye[2] = dirAux[2];
	renderer.setDirLightMode(dayMode, dirEye);

	// Set point light mode (candles) and transform the candle positions to eye space
	float candleEye[6][4];
	for (int i = 0; i < 6; i++) {
		float aux[4];
		mu.multMatrixPoint(gmu::VIEW, candlePos[i], aux);
		candleEye[i][0] = aux[0]; candleEye[i][1] = aux[1]; candleEye[i][2] = aux[2]; candleEye[i][3] = aux[3];
	}
	renderer.setPointLightMode(candleMode, candleEye);

	// Set spot light mode (headlights) and transform the spotlight positions and direction to eye space
	float DEG2RAD = 3.14159265f / 180.0f; // Conversion factor from degrees to radians

	float carAngleRad = carBarbie.angle * DEG2RAD;
	float cosA = cosf(carAngleRad), sinA = sinf(carAngleRad); // Calculate the cosine and sine of the car's angle in radians

	// Tilt the headlights downwards so that they illuminate the road ahead
	float tiltRad = -15.0f * DEG2RAD;
	float cosT = cosf(tiltRad), sinT = sinf(tiltRad); // Calculate the cosine and sine of the tilt angle in radians

	// Calculate the forward direction of the headlights in world space, taking into account the car's orientation and the tilt angle
	float fwdWorld[4] = { sinA * cosT, sinT, cosA * cosT, 0.0f };
	float spotDirEyeAux[4];
	mu.multMatrixPoint(gmu::VIEW, fwdWorld, spotDirEyeAux);
	float spotDirEye[3] = { spotDirEyeAux[0], spotDirEyeAux[1], spotDirEyeAux[2] };

	// Same local positions for the left and right headlights
	float localPos[2][3] = {
		{ -1.7f, 1.18f, 3.3f }, // left
		{  1.7f, 1.18f, 3.3f }  // right
	};

	float spotPosEye[2][4];
	for (int i = 0; i < 2; i++) {
		float lx = localPos[i][0], ly = localPos[i][1], lz = localPos[i][2];

		// Transform the local headlight position to world space, taking into account the car's position and orientation
		float posWorld[4] = {
			carBarbie.x + lx * cosA + lz * sinA,
			carBarbie.y + ly,
			carBarbie.z - lx * sinA + lz * cosA,
			1.0f
		};
		mu.multMatrixPoint(gmu::VIEW, posWorld, spotPosEye[i]);
	}

	float cosCutOff = cosf(spotCosCutOff * DEG2RAD);
	renderer.setSpotLightMode(headlightMode, spotPosEye, spotDirEye, cosCutOff, spotEx);

	// Draw the table
	drawObject(TABLE_MESH, 0.0f,  tablePosY, 0.0f, tableWidth, tableHeight, tableDepth);

	// Draw the roads
	drawObject(ROAD_MESH, 70.0f,  roadPosY, -5.0f,  roadWidth, roadHeight, 90.0f,	  0.0f, 4); // 1: Start road
	drawObject(ROAD_MESH, 40.0f,  roadPosY, 50.0f,  80.0f,     roadHeight, roadWidth, 0.0f, 4); // 2: Horizontal road
	drawObject(ROAD_MESH, -5.0f,  roadPosY, 30.0f,  50.0f,	   roadHeight, roadWidth, 0.0f, 4);	// 3: Horizontal road
	drawObject(ROAD_MESH, -20.0f, roadPosY, 50.0f,  roadWidth, roadHeight, 20.0f,     0.0f, 4); // 4: Vertical road
	drawObject(ROAD_MESH, -45.0f, roadPosY, 70.0f,  70.0f,	   roadHeight, roadWidth, 0.0f, 4); // 5: Horizontal road
	drawObject(ROAD_MESH, -70.0f, roadPosY, 0.0f,   roadWidth, roadHeight, 120.0f,    0.0f, 4); // 6: Vertical road
	drawObject(ROAD_MESH, -35.0f, roadPosY, -70.0f, 90.0f,	   roadHeight, roadWidth, 0.0f, 4); // 7: Horizontal road
	drawObject(ROAD_MESH, 0.0f,   roadPosY, -35.0f, roadWidth, roadHeight, 50.0f,	  0.0f, 4); // 8: Vertical road
	drawObject(ROAD_MESH, 20.0f,  roadPosY, 0.0f,   60.0f,     roadHeight, roadWidth, 0.0f, 4); // 9: Horizontal road
	drawObject(ROAD_MESH, 40.0f,  roadPosY, -30.0f, roadWidth, roadHeight, 40.0f,	  0.0f, 4); // 10: Vertical road
	drawObject(ROAD_MESH, 55.0f,  roadPosY, -60.0f, 50.0f,     roadHeight, roadWidth, 0.0f, 4); // 11: Horizontal road

	// Draw the margins
	drawObject(MARGIN_MESH, 60.0f,  marginPosY, -5.0f,  marginWidth, marginHeight, 90.0f);       // 1.1: Left start road margin
	drawObject(MARGIN_MESH, 80.0f,  marginPosY, -5.0f,  marginWidth, marginHeight, 130.0f);      // 1.2: Right start road margin
	drawObject(MARGIN_MESH, 40.0f,  marginPosY, 40.0f,  40.0f,       marginHeight, marginWidth); // 2.1: Front horizontal road margin
	drawObject(MARGIN_MESH, 40.0f,  marginPosY, 60.0f,  80.0f,       marginHeight, marginWidth); // 2.2: Back horizontal road maring
	drawObject(MARGIN_MESH, 0.0f,   marginPosY, 50.0f,  marginWidth, marginHeight, 20.0f);       // 2.3: Left horizontal road margin
	drawObject(MARGIN_MESH, -5.0f,  marginPosY, 20.0f,  50.0f,	     marginHeight, marginWidth); // 3.1: Front horizontal road margin
	drawObject(MARGIN_MESH, -5.0f,  marginPosY, 40.0f,  10.0f,	     marginHeight, marginWidth); // 3.2: Back horizontal road margin
	drawObject(MARGIN_MESH, 20.0f,  marginPosY, 30.0f,  marginWidth, marginHeight, 20.0f);	     // 3.3: Right horizontal road margin
	drawObject(MARGIN_MESH, -30.0f, marginPosY, 40.0f,  marginWidth, marginHeight, 40.0f);	     // 4.1: Left vertical road margin
	drawObject(MARGIN_MESH, -10.0f, marginPosY, 60.0f,  marginWidth, marginHeight, 40.0f);	     // 4.2: Right vertical road margin
	drawObject(MARGIN_MESH, -45.0f, marginPosY, 60.0f,  30.0f,       marginHeight, marginWidth); // 5.1: Front horizontal road margin
	drawObject(MARGIN_MESH, -45.0f, marginPosY, 80.0f,  70.0f,       marginHeight, marginWidth); // 5.2: Back horizontal road margin
	drawObject(MARGIN_MESH, -80.0f, marginPosY, 0.0f,   marginWidth, marginHeight, 160.0f);	     // 6.1: Left vertical road margin
	drawObject(MARGIN_MESH, -60.0f, marginPosY, 0.0f,   marginWidth, marginHeight, 120.0f);	     // 6.2: Right vertical road margin
	drawObject(MARGIN_MESH, -35.0f, marginPosY, -80.0f, 90.0f,       marginHeight, marginWidth); // 7.1: Front horizontal road margin
	drawObject(MARGIN_MESH, -35.0f, marginPosY, -60.0f, 50.0f,       marginHeight, marginWidth); // 7.2: Back horizontal road margin
	drawObject(MARGIN_MESH, -10.0f, marginPosY, -25.0f, marginWidth, marginHeight, 70.0f);	     // 8.1: Left vertical road margin
	drawObject(MARGIN_MESH, 10.0f,  marginPosY, -45.0f, marginWidth, marginHeight, 70.0f);	     // 8.2: Right vertical road margin
	drawObject(MARGIN_MESH, 20.0f,  marginPosY, -10.0f, 20.0f,       marginHeight, marginWidth); // 9.1: Front horizontal road margin
	drawObject(MARGIN_MESH, 20.0f,  marginPosY, 10.0f,  60.0f,       marginHeight, marginWidth); // 9.2: Back horizontal road margin
	drawObject(MARGIN_MESH, 30.0f,  marginPosY, -40.0f, marginWidth, marginHeight, 60.0f);	     // 10.1: Left vertical road margin
	drawObject(MARGIN_MESH, 50.0f,  marginPosY, -20.0f, marginWidth, marginHeight, 60.0f);	     // 10.2: Right vertical road margin
	drawObject(MARGIN_MESH, 55.0f,  marginPosY, -70.0f, 50.0f,       marginHeight, marginWidth); // 11.1: Front horizontal road margin
	drawObject(MARGIN_MESH, 55.0f,  marginPosY, -50.0f, 10.0f,       marginHeight, marginWidth); // 11.2: Back horizontal road margin

	// Draw the start flag and start line
	drawObject(ROAD_MESH,   60.0f, roadPosY + 7.5f,  -5.0f, 1.0f,      15.0f, 1.0f); // 1: Left flag pole
	drawObject(ROAD_MESH,   80.0f, roadPosY + 7.5f,  -5.0f, 1.0f,      15.0f, 1.0f); // 2: Right flag pole
	drawObject(MARGIN_MESH, 70.0f, roadPosY + 10.0f, -5.0f, roadWidth, 3.0f,  1.0f); // 3: Flag
	drawObject(MARGIN_MESH, 70.0f, roadPosY + 0.1f,  -5.0f, roadWidth, 0.1f,  1.0f); // 4: Start line

	// Draw the 6 candles
	drawCenteredObject(CANDLE_BASE_MESH, 51.25f, candleBasePosY, 32.25f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0); // 1.1: Candle base
	drawCenteredObject(CANDLE_WICK_MESH, 51.25f, candleWickPosY, 32.25f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0); // 1.2: Candle wick
	drawCenteredObject(CANDLE_BASE_MESH, -5.5f,  candleBasePosY, 48.5f,  1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0); // 2.1: Candle base
	drawCenteredObject(CANDLE_WICK_MESH, -5.5f,  candleWickPosY, 48.5f,  1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0); // 2.2: Candle wick
	drawCenteredObject(CANDLE_BASE_MESH, -45.5f, candleBasePosY, 50.5f,  1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0); // 3.1: Candle base
	drawCenteredObject(CANDLE_WICK_MESH, -45.5f, candleWickPosY, 50.5f,  1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0); // 3.2: Candle wick
	drawCenteredObject(CANDLE_BASE_MESH, -35.5f, candleBasePosY, -50.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0); // 4.1: Candle base
	drawCenteredObject(CANDLE_WICK_MESH, -35.5f, candleWickPosY, -50.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0); // 4.2: Candle wick
	drawCenteredObject(CANDLE_BASE_MESH, 20.5f,  candleBasePosY, -19.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0); // 5.1: Candle base
	drawCenteredObject(CANDLE_WICK_MESH, 20.5f,  candleWickPosY, -19.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0); // 5.2: Candle wick
	drawCenteredObject(CANDLE_BASE_MESH, 55.5f,  candleBasePosY, -41.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0); // 6.1: Candle base
	drawCenteredObject(CANDLE_WICK_MESH, 55.5f,  candleWickPosY, -41.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0); // 6.2: Candle wick


	// Draw butters
	for (int i = 0; i < NUM_BUTTERS; i++) {
		drawButter(butters[i]);
	}

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

		case 'v':
		case 'V':
			printf("Camera Spherical Coordinates (%f, %f, %f)\n", alpha, _beta, r);
			break;

		case 'n':
		case 'N':
			dayMode = !dayMode;
			printf("Day mode: %s\n", dayMode ? "ON" : "OFF");
			break;

		case 'c':
		case 'C':
			candleMode = !candleMode;
			printf("Candle mode: %s\n", candleMode ? "ON" : "OFF");
			break;

		case 'h':
		case 'H':
			headlightMode = !headlightMode;
			printf("Headlights: %s\n", headlightMode ? "ON" : "OFF");
			break;

		case 'r':    //reset
		case 'R':
			alpha = 57.0f; _beta = 18.0f;  // Camera Spherical Coordinates
			r = 45.0f;
			camX = r * sin(alpha * 3.14f / 180.0f) * cos(_beta * 3.14f / 180.0f);
			camZ = r * cos(alpha * 3.14f / 180.0f) * cos(_beta * 3.14f / 180.0f);
			camY = r * sin(_beta * 3.14f / 180.0f);
			break;

		case 'j':
		case 'J':
			glEnable(GL_MULTISAMPLE); break;
		case 'k':
		case 'K':
			glDisable(GL_MULTISAMPLE); break;
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
// 0: table (cube)
// 1: road (cube)
// 2: margin (cube)
// 4: car body (cube)
// 5: metal car parts (cube)
// 6: car window (cube)
// 7: car wheel (torus)
// 8: car headlight (sphere)
// 9: car plate (cube)
// 10: candle base (cylinder)
// 11: candle wick (cylinder)

void buildScene()
{
	//Texture Object definition
	renderer.TexObjArray.texture2D_Loader("assets/stone.tga");
	renderer.TexObjArray.texture2D_Loader("assets/checker.png");
	renderer.TexObjArray.texture2D_Loader("assets/lightwood.tga");
	renderer.TexObjArray.texture2D_Loader("assets/road.jpg");

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

	// create geometry and VAO of the table (0)
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

	// create geometry and VAO of the road (1)
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

	// create geometry and VAO of the road margins (2)
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

	// create geometry and VAO for the car body (3)
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

	// create geometry and VAO for the metal car parts (4)
	float ambBumper[] = { 0.04f, 0.04f, 0.05f, 1.0f };
	float diffBumper[] = { 0.16f, 0.17f, 0.19f, 1.0f };
	float specBumper[] = { 0.85f, 0.88f, 0.95f, 1.0f };
	amesh = createCube();
	memcpy(amesh.mat.ambient, ambBumper, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffBumper, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specBumper, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = 180.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO for the car window (5)
	float ambGlass[] = { 0.08f, 0.12f, 0.16f, 1.0f };
	float diffGlass[] = { 0.35f, 0.65f, 0.85f, 1.0f };
	float specGlass[] = { 0.80f, 0.90f, 1.00f, 1.0f };
	amesh = createCube();
	memcpy(amesh.mat.ambient, ambGlass, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffGlass, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specGlass, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = 220.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO for the car wheels (6)
	float ambWheel[] = { 0.02f, 0.02f, 0.02f, 1.0f };
	float diffWheel[] = { 0.06f, 0.06f, 0.07f, 1.0f };
	float specWheel[] = { 0.20f, 0.20f, 0.22f, 1.0f };
	amesh = createTorus(0.38f, 0.90f, 20, 20);
	memcpy(amesh.mat.ambient, ambWheel, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffWheel, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specWheel, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = 40.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO for the car headlights (7)
	float ambHeadlight[] = { 0.35f, 0.35f, 0.30f, 1.0f };
	float diffHeadlight[] = { 1.00f, 0.95f, 0.80f, 1.0f };
	float specHeadlight[] = { 1.00f, 1.00f, 1.00f, 1.0f };
	float emissiveHeadlight[] = { 0.25f, 0.23f, 0.18f, 1.0f };
	amesh = createSphere(1.0f, 20);
	memcpy(amesh.mat.ambient, ambHeadlight, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffHeadlight, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specHeadlight, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissiveHeadlight, 4 * sizeof(float));
	amesh.mat.shininess = 200.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO for the car plate (8)
	float ambPlate[] = { 0.30f, 0.30f, 0.30f, 1.0f };
	float diffPlate[] = { 0.90f, 0.90f, 0.90f, 1.0f };
	float specPlate[] = { 0.25f, 0.25f, 0.25f, 1.0f };
	amesh = createCube();
	memcpy(amesh.mat.ambient, ambPlate, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffPlate, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specPlate, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = 40.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO for the candle base (9)
	float ambWax[] = { 0.35f, 0.32f, 0.25f, 1.0f };
	float diffWax[] = { 0.95f, 0.90f, 0.75f, 1.0f };
	float specWax[] = { 0.30f, 0.30f, 0.25f, 1.0f };
	amesh = createCylinder(12.0f, 2.0f, 20);
	memcpy(amesh.mat.ambient, ambWax, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffWax, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specWax, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = 15.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO for the candle wick (10)
	float ambWick[] = { 0.05f, 0.04f, 0.03f, 1.0f };
	float diffWick[] = { 0.15f, 0.10f, 0.08f, 1.0f };
	float specWick[] = { 0.10f, 0.10f, 0.10f, 1.0f };
	amesh = createCylinder(3.0f, 0.3f, 12);
	memcpy(amesh.mat.ambient, ambWick, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffWick, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specWick, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = 10.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// Butter (11)
	float ambButterYellow[] = { 0.35f, 0.30f, 0.02f, 1.0f };
	float diffButterYellow[] = { 1.00f, 0.85f, 0.05f, 1.0f };
	float specButterYellow[] = { 0.20f, 0.20f, 0.10f, 1.0f };
	amesh = createCube();
	memcpy(amesh.mat.ambient, ambButterYellow, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffButterYellow, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specButterYellow, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = 30.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// Butter - beige part of the package (12)
	float ambButterBeige[] = { 0.30f, 0.27f, 0.16f, 1.0f };
	float diffButterBeige[] = { 0.90f, 0.82f, 0.55f, 1.0f };
	float specButterBeige[] = { 0.20f, 0.20f, 0.15f, 1.0f };
	amesh = createCube();
	memcpy(amesh.mat.ambient, ambButterBeige, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffButterBeige, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specButterBeige, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = 30.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);


	// Butter - blue part of the package (13)
	float ambButterBlue[] = { 0.05f, 0.07f, 0.30f, 1.0f };
	float diffButterBlue[] = { 0.10f, 0.18f, 0.85f, 1.0f };
	float specButterBlue[] = { 0.20f, 0.20f, 0.30f, 1.0f };
	amesh = createCube();
	memcpy(amesh.mat.ambient, ambButterBlue, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffButterBlue, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specButterBlue, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = 30.0f;
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

