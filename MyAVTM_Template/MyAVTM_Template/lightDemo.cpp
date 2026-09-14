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

void changeSize(int w, int h) {

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

	// load identity matrices
	mu.loadIdentity(gmu::VIEW);
	mu.loadIdentity(gmu::MODEL);
	// set the camera using a function similar to gluLookAt
	mu.lookAt(camX, camY, camZ, 0, 0, 0, 0, 1, 0);

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

	//Render text (bitmap fonts) in screen coordinates. So use ortoghonal projection with viewport coordinates.
	//Each glyph quad texture needs just one byte color channel: 0 in background and 1 for the actual character pixels. Use it for alpha blending
	//text to be rendered in last place to be in front of everything
	
	//if(fontLoaded) {
	//	glDisable(GL_DEPTH_TEST);
	//	TextCommand textCmd = { "AVTM 2026 Welcome:\nGood Luck!", {100, 100}, 0.5 };
	//	//the glyph contains transparent background colors and non-transparent for the actual character pixels. So we use the blending
	//	glEnable(GL_BLEND);  
	//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//	int m_viewport[4];
	//	glGetIntegerv(GL_VIEWPORT, m_viewport);

	//	//viewer at origin looking down at  negative z direction

	//	mu.loadIdentity(gmu::MODEL);
	//	mu.loadIdentity(gmu::VIEW);
	//	mu.pushMatrix(gmu::PROJECTION);
	//	mu.loadIdentity(gmu::PROJECTION);
	//	mu.ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1], m_viewport[1] + m_viewport[3] - 1, -1, 1);
	//	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
	//	textCmd.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
	//	renderer.renderText(textCmd);
	//	mu.popMatrix(gmu::PROJECTION);
	//	glDisable(GL_BLEND);
	//	glEnable(GL_DEPTH_TEST);
	//	
	//}
	
	glutSwapBuffers();
}

// ------------------------------------------------------------
//
// Events from the Keyboard
//

void processKeys(unsigned char key, int xx, int yy)
{
	switch(key) {

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



