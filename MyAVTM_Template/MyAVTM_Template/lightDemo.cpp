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
#include <algorithm>
#include <cstdlib>
#include <ctime>

// Include GLEW to access OpenGL 3.3 functions
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

// ============================================================================
// GLOBAL CONFIGURATION
// ============================================================================

#define CAPTION "AVTM 2026 Micro Machines 3D"
#define FPS 60

int WindowHandle = 0;
int WinX = 640, WinY = 480;
float aspectRatio = 640.0f / 480.0f;
unsigned int FrameCount = 0;

// Game state variables
int lives = 5;
int points = 0;
bool paused = false;

// Font file path
const string fontPathFile = "fonts/arial.ttf";
bool fontLoaded = false;

// Engine state variables
gmu mu; // Object of class mathUtility to manage the model, view and projection matrices
Renderer renderer; // Object of class Renderer to manage the rendering of meshes and textures
	
// Camera position and orientation
float camX, camY, camZ;
float alpha = 57.0f, _beta = 18.0f;
float r = 45.0f;

// Cameras
// 1 = Fixed satellite orthographic
// 2 = Fixed satellite perspective
// 3 = Car following perspective
int CameraMode = 1;

// Mouse Tracking Variables
int startX, startY, tracking = 0;

// Car initial position and orientation
const float CAR_START_X = 70.0f;
const float CAR_START_Z = -15.0f;
const float CAR_START_ANGLE = 0.0f;

// Movement keys state
bool keyFrente = false;
bool keyTras = false;
bool keyDir = false;
bool keyEsq = false;

// Off-table fall state variables
bool carFalling = false;
float fallTimer = 0.0f;
float fallVelocityY = 0.0f;
float fallDirX = 0.0f;
float fallDirZ = 0.0f;
float fallHorizontalSpeed = 0.0f;
const float FALL_GRAVITY = 35.0f;
const float FALL_RESPAWN_TIME = 2.2f;

// Time tracking variables
long myTime, timebase = 0, frame = 0;

// Directional light (Day/Night)
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

// Constants for mesh IDs
const int TABLE_MESH		 = 0; // table
const int ROAD_MESH			 = 1; // road
const int MARGIN_MESH		 = 2; // margin
const int CAR_NORMAL_MESH	 = 3; // pink
const int CAR_METAL_MESH	 = 4; // metal
const int CAR_GLASS_MESH	 = 5; // gray-blue glass
const int CAR_WHEEL_MESH	 = 6; // tires
const int CAR_LIGHT_MESH	 = 7; // headlights
const int CAR_PLATE_MESH	 = 8; // plate
const int CANDLE_BASE_MESH	 = 9; // candle base
const int CANDLE_WICK_MESH	 = 10; // candle wick
const int BUTTER_YELLOW_MESH = 11; // butter
const int BUTTER_BEIGE_MESH  = 12; // beige part of the butter package
const int BUTTER_BLUE_MESH   = 13; // blue part of the butter package
const int ORANGE_MESH		 = 14; // orange
const int ORANGE_BLACK_MESH  = 15; // black part of the orange
const int CHEERIO_MESH		 = 16; // cheerio

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// Car structure

struct Car {
	// World position and orientation
	float x = CAR_START_X;
	float y = 0.0f;
	float z = CAR_START_Z;
	float angle = CAR_START_ANGLE;
	float dir[3] = { 0.0f, 0.0f, 1.0f };

	// Fall animation state
	float fallPitch = 0.0f;
	float fallRoll = 0.0f;

	// Dimensions
	float width = 4.5f;
	float height = 3.15f;
	float depth = 7.0f;

	// Movement state
	float speed = 0.0f;
	float acceleration = 20.0f;
	float maxSpeed = 80.0f;

	// Wheel and steering state
	float wheelSpin = 0.0f;   // Wheel rotation 
	float steerVisual = 0.0f; // Wheel visual steering angle
};

Car carBarbie;

struct CarMeshIDs {
	int paint, trim, trimCyl, chrome, chromeCyl, rubber;
	int interior, seat, headlight, taillight, plate, plateBlue;
	int glass, glassTri, mirror, steering;
};

CarMeshIDs carMesh;

struct GlassPiece {
	int mesh;
	bool box;
	float x, y, z;
	float sx, sy, sz;
	float rotX, rotZ;
	float cx, cy, cz;
	float depth;
};

// Car design constants
// belt = window base
const float CAR_BELT = 1.85f;
const float CAR_ROOF = 2.85f;
const float CAR_WS_BASE = 1.45f, CAR_WS_TOP = 0.50f; // Windshield bounds (z)
const float CAR_RW_BASE = -1.45f, CAR_RW_TOP = -0.65f; // Rear window bounds (z)
const float CAR_GLASS_X = 2.15f;
const float CAR_WHEEL_R = 0.80f; // Wheel radius

// Butter structure

struct Butter {
	float x;
	float z;
};

Butter butters[] = {
	{ 70.0f, 25.0f },  // Road 1
	{ 50.0f, 50.0f },  // Road 2
	{ 10.0f, 30.0f },  // Road 3
	{-20.0f, 50.0f },  // Road 4
	{-45.0f, 70.0f },  // Road 5
	{-70.0f, 30.0f },  // Road 6
	{-70.0f, -35.0f }, // Road 6
	{-40.0f, -70.0f }, // Road 7
	{ 0.0f, -35.0f },  // Road 8
	{ 20.0f, 0.0f },   // Road 9
	{ 40.0f, -30.0f }, // Road 10
	{ 60.0f, -60.0f }  // Road 11
};

const int NUM_BUTTERS = sizeof(butters) / sizeof(butters[0]);

// Orange structure

struct Orange {
	float x;
	float z;
	float dirX;
	float dirZ;
	float speed;
	float acceleration;
	float angle;
};

Orange oranges[] = {
	{-60.0f, -55.0f,  1.0f,  0.0f, 0.06f, 0.000005f, 0.0f},
	{ 60.0f,  15.0f, -1.0f,  0.0f, 0.08f, 0.000005f, 0.0f},
	{-55.0f,  60.0f,  0.0f, -1.0f, 0.10f, 0.000005f, 0.0f},
	{ 45.0f, -60.0f,  0.0f,  1.0f, 0.12f, 0.000005f, 0.0f}
};

const int NUM_ORANGES = sizeof(oranges) / sizeof(oranges[0]);

// Cheerios structure

struct CheeriosLine {
	float x1, z1;
	float x2, z2;
};

CheeriosLine cheeriosLine[] = {
	{ 60.0f, -50.0f, 60.0f,  40.0f }, // 1.1 margin
	{ 80.0f, -70.0f, 80.0f,  60.0f }, // 1.2 margin
	{ 20.0f,  40.0f, 60.0f,  40.0f }, // 2.1 margin
	{  0.0f,  60.0f, 80.0f,  60.0f }, // 2.2 margin
	{  0.0f,  40.0f,  0.0f,  60.0f }, // 2.3 margin
	{-30.0f,  20.0f, 20.0f,  20.0f }, // 3.1 margin
	{-10.0f,  40.0f,  0.0f,  40.0f }, // 3.2 margin
	{ 20.0f,  20.0f, 20.0f,  40.0f }, // 3.3 margin
	{-30.0f,  20.0f,-30.0f,  60.0f }, // 4.1 margin
	{-10.0f,  40.0f,-10.0f,  80.0f }, // 4.2 margin
	{-60.0f,  60.0f,-30.0f,  60.0f }, // 5.1 margin
	{-80.0f,  80.0f,-10.0f,  80.0f }, // 5.2 margin
	{-80.0f, -80.0f,-80.0f,  80.0f }, // 6.1 margin
	{-60.0f, -60.0f,-60.0f,  60.0f }, // 6.2 margin
	{-80.0f, -80.0f, 10.0f, -80.0f }, // 7.1 margin
	{-60.0f, -60.0f,-10.0f, -60.0f }, // 7.2 margin
	{-10.0f, -60.0f,-10.0f,  10.0f }, // 8.1 margin
	{ 10.0f, -80.0f, 10.0f, -10.0f }, // 8.2 margin
	{ 10.0f, -10.0f, 30.0f, -10.0f }, // 9.1 margin
	{-10.0f,  10.0f, 50.0f,  10.0f }, // 9.2 margin
	{ 30.0f, -70.0f, 30.0f, -10.0f }, // 10.1 margin
	{ 50.0f, -50.0f, 50.0f,  10.0f }, // 10.2 margin
	{ 30.0f, -70.0f, 80.0f, -70.0f }, // 11.1 margin
	{ 50.0f, -50.0f, 60.0f, -50.0f }  // 11.2 margin
};

struct CheerioInstance {
	float x;
	float z;
};

vector<CheerioInstance> cheerioInstances; // Vector to store the positions of cheerios along the lines

const int NUM_CHEERIOS_LINES = sizeof(cheeriosLine) / sizeof(cheeriosLine[0]);

// Other structures

struct AABB {
	float minX, maxX;
	float minZ, maxZ;
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Draw a mesh object with the given position, scale, and mesh ID, centered at its local origin
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
	data.texMode = texMode; // 0: No texturing; 1: Modulate diffuse color with texel color; 2: Diffuse color is replaced by texel color; 3: Multitexturing
	data.vm = mu.get(gmu::VIEW_MODEL),
		data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
	data.normal = mu.getNormalMatrix();
	renderer.renderMesh(data);
	mu.popMatrix(gmu::MODEL);
}

// Draw a mesh object with the given position, scale, rotation, and mesh ID, centered at the origin
void drawCenteredObject(
	int meshID,
	float posX, float posY, float posZ,
	float scaleX, float scaleY, float scaleZ,
	float rotX = 0.0f, float rotY = 0.0f, float rotZ = 0.0f,
	int texMode = 0)
{
	mu.pushMatrix(gmu::MODEL);

	mu.translate(gmu::MODEL, posX, posY, posZ);

	mu.rotate(gmu::MODEL, rotX, 1.0f, 0.0f, 0.0f); // Rotate around X-axis
	mu.rotate(gmu::MODEL, rotY, 0.0f, 1.0f, 0.0f); // Rotate around Y-axis
	mu.rotate(gmu::MODEL, rotZ, 0.0f, 0.0f, 1.0f); // Rotate around Z-axis

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

// Helper to register car submeshes with their material properties
static int addCarMesh(MyMesh m, const float amb[4], const float diff[4], const float spec[4], float shininess) {
	const float noEmissive[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	memcpy(m.mat.ambient, amb, 4 * sizeof(float));
	memcpy(m.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(m.mat.specular, spec, 4 * sizeof(float));
	memcpy(m.mat.emissive, noEmissive, 4 * sizeof(float));
	m.mat.shininess = shininess;
	m.mat.texCount = 0;
	renderer.myMeshes.push_back(m);
	return (int)renderer.myMeshes.size() - 1;
}

// Build the car meshes with their respective materials and properties
void buildCarMeshes()
{
	// Pink paint (cilinder, sphere and torus)
	float paintA[] = { 0.25f, 0.03f, 0.15f, 1.0f };
	float paintD[] = { 0.95f, 0.20f, 0.60f, 1.0f };
	float paintS[] = { 0.90f, 0.80f, 0.90f, 1.0f };
	carMesh.paint = addCarMesh(createCube(), paintA, paintD, paintS, 100.0f);

	// Glossy black plastic (bumpers, grille, wheels, pillar B, rims)
	float trimA[] = { 0.03f, 0.03f, 0.03f, 1.0f };
	float trimD[] = { 0.08f, 0.08f, 0.09f, 1.0f };
	float trimS[] = { 0.50f, 0.50f, 0.55f, 1.0f };
	carMesh.trim = addCarMesh(createCube(), trimA, trimD, trimS, 60.0f);
	carMesh.trimCyl = addCarMesh(createCylinder(1.0f, 1.0f, 32), trimA, trimD, trimS, 60.0f);
	carMesh.steering = addCarMesh(createTorus(0.8f, 1.0f, 32, 12), trimA, trimD, trimS, 60.0f);

	// Chrome metal (handles, inner rims, exhaust pipe)
	float chromeA[] = { 0.25f, 0.25f, 0.27f, 1.0f };
	float chromeD[] = { 0.55f, 0.56f, 0.60f, 1.0f };
	float chromeS[] = { 1.00f, 1.00f, 1.00f, 1.0f };
	carMesh.chrome = addCarMesh(createCube(), chromeA, chromeD, chromeS, 180.0f);
	carMesh.chromeCyl = addCarMesh(createCylinder(1.0f, 1.0f, 32), chromeA, chromeD, chromeS, 180.0f);

	// Rubber tires (torus)
	float rubberA[] = { 0.02f, 0.02f, 0.02f, 1.0f };
	float rubberD[] = { 0.05f, 0.05f, 0.05f, 1.0f };
	float rubberS[] = { 0.15f, 0.15f, 0.15f, 1.0f };
	carMesh.rubber = addCarMesh(createTorus(0.45f, 1.0f, 32, 16), rubberA, rubberD, rubberS, 10.0f);

	// Old pink (interior dashboard, door panels, floor)
	float interiorA[] = { 0.35f, 0.24f, 0.28f, 1.0f };
	float interiorD[] = { 0.85f, 0.60f, 0.68f, 1.0f };
	float interiorS[] = { 0.15f, 0.15f, 0.15f, 1.0f };
	carMesh.interior = addCarMesh(createCube(), interiorA, interiorD, interiorS, 15.0f);

	// Creme almost white (seats)
	float seatA[] = { 0.55f, 0.52f, 0.48f, 1.0f };
	float seatD[] = { 0.97f, 0.94f, 0.88f, 1.0f };
	float seatS[] = { 0.20f, 0.20f, 0.20f, 1.0f };
	carMesh.seat = addCarMesh(createCube(), seatA, seatD, seatS, 20.0f);

	// Lights (oblate sphere), taillights and plates (with the little blue detail)
	float headA[] = { 0.60f, 0.60f, 0.55f, 1.0f };
	float headD[] = { 1.00f, 0.97f, 0.85f, 1.0f };
	float headS[] = { 1.00f, 1.00f, 1.00f, 1.0f };
	carMesh.headlight = addCarMesh(createSphere(1.0f, 20), headA, headD, headS, 200.0f);

	float tailA[] = { 0.40f, 0.00f, 0.00f, 1.0f };
	float tailD[] = { 0.90f, 0.05f, 0.05f, 1.0f };
	float tailS[] = { 0.80f, 0.60f, 0.60f, 1.0f };
	carMesh.taillight = addCarMesh(createCube(), tailA, tailD, tailS, 100.0f);

	float plateA[] = { 0.30f, 0.30f, 0.30f, 1.0f };
	float plateD[] = { 0.90f, 0.90f, 0.90f, 1.0f };
	float plateS[] = { 0.25f, 0.25f, 0.25f, 1.0f };
	carMesh.plate = addCarMesh(createCube(), plateA, plateD, plateS, 40.0f);

	float euA[] = { 0.02f, 0.05f, 0.25f, 1.0f };
	float euD[] = { 0.05f, 0.15f, 0.75f, 1.0f };
	float euS[] = { 0.25f, 0.25f, 0.25f, 1.0f };
	carMesh.plateBlue = addCarMesh(createCube(), euA, euD, euS, 40.0f);

	// Transparent glass (alpha: opacity; color: ambient; specular: weak)
	float glassA[] = { 0.30f, 0.50f, 0.75f, 0.25f };
	float glassD[] = { 0.20f, 0.35f, 0.55f, 0.25f };
	float glassS[] = { 0.35f, 0.35f, 0.40f, 0.25f };
	carMesh.glass = addCarMesh(createCube(), glassA, glassD, glassS, 120.0f);
	// Cilinder with 3 sides = triangular prism: corners of the windows next to pillars A and C
	carMesh.glassTri = addCarMesh(createCylinder(1.0f, 1.0f, 3), glassA, glassD, glassS, 120.0f);

	// Side mirrors (more blue and less transparent)
	// TO DO: refletir (Esperar pelo enunciado)
	float mirrorA[] = { 0.35f, 0.55f, 0.85f, 0.55f };
	float mirrorD[] = { 0.35f, 0.55f, 0.90f, 0.55f };
	float mirrorS[] = { 0.80f, 0.80f, 0.90f, 0.55f };
	carMesh.mirror = addCarMesh(createCube(), mirrorA, mirrorD, mirrorS, 200.0f);
}

// Helpers for car positioning and drawing, using the car's mesh IDs and dimensions
static void carBox(int mesh, float x, float y, float z, float sx, float sy, float sz, float rotX = 0.0f) {
	drawObject(mesh, x, y, z, sx, sy, sz, rotX, 0);   // 0: sem textura
}

static void carShape(int mesh, float x, float y, float z, float sx, float sy, float sz,
	float rx = 0.0f, float ry = 0.0f, float rz = 0.0f) {
	drawCenteredObject(mesh, x, y, z, sx, sy, sz, rx, ry, rz, 0);
}

// Computes spacial positioning and X rotation for sloped surfaces
static void slopeParams(float yA, float zA, float yB, float zB,
	float& yc, float& zc, float& len, float& angDeg)
{
	float dy = yB - yA, dz = zB - zA;
	yc = 0.5f * (yA + yB);
	zc = 0.5f * (zA + zB);
	len = sqrtf(dy * dy + dz * dz);
	angDeg = atan2f(dz, dy) * 180.0f / 3.14159265f;
}

// ============================================================================
// CAR DRAWING FUNCTIONS
// ============================================================================

// Car base, structure, and exterior details (all opaque)
static void drawCarBody() {
	const int P = carMesh.paint, T = carMesh.trim;
	float yc, zc, len, ang;

	carBox(carMesh.interior, 0.0f, 0.45f, 0.0f, 4.18f, 0.20f, 3.00f); // Car floor
	carBox(T, 0.0f, 0.90f, 2.35f, 3.00f, 1.10f, 1.70f); // Front floor block
	carBox(T, 0.0f, 0.90f, -2.35f, 3.00f, 1.10f, 1.70f); // Back floor block
	carBox(T, 0.0f, 0.90f, 1.45f, 4.18f, 1.10f, 0.10f); // Roof front
	carBox(T, 0.0f, 0.90f, -1.45f, 4.18f, 1.10f, 0.10f); // Roof back

	carBox(P, 2.17f, 1.10f, 0.0f, 0.16f, 1.50f, 3.00f); // Left doors
	carBox(P, -2.17f, 1.10f, 0.0f, 0.16f, 1.50f, 3.00f); // Right doors
	carBox(P, 0.0f, 1.65f, 2.45f, 4.50f, 0.40f, 2.10f); // Bonnet
	carBox(P, 0.0f, 1.65f, -2.45f, 4.50f, 0.40f, 2.10f); // Baggage
	carBox(P, 0.0f, 1.20f, 3.35f, 4.50f, 0.50f, 0.30f); // Front vertical
	carBox(P, 0.0f, 1.20f, -3.35f, 4.50f, 0.50f, 0.30f); // Rear vertical

	carBox(P, 0.0f, CAR_ROOF + 0.05f, -0.07f, 4.40f, 0.10f, 1.30f); // Roof top
	slopeParams(CAR_BELT, CAR_WS_BASE, CAR_ROOF, CAR_WS_TOP, yc, zc, len, ang);
	carBox(P, 2.14f, yc, zc, 0.14f, len, 0.14f, ang); // Left pillar A
	carBox(P, -2.14f, yc, zc, 0.14f, len, 0.14f, ang); // Right pillar A
	slopeParams(CAR_BELT, CAR_RW_BASE, CAR_ROOF, CAR_RW_TOP, yc, zc, len, ang);
	carBox(P, 2.14f, yc, zc, 0.14f, len, 0.14f, ang); // Left pillar C
	carBox(P, -2.14f, yc, zc, 0.14f, len, 0.14f, ang); // Right pillar C
	carBox(T, 2.16f, 2.35f, -0.10f, 0.12f, 1.00f, 0.20f); // Left pillar B
	carBox(T, -2.16f, 2.35f, -0.10f, 0.12f, 1.00f, 0.20f); // Right pillar B

	carBox(P, 0.0f, 0.675f, 3.375f, 4.40f, 0.55f, 0.35f); // Front bumper
	carBox(T, 0.0f, 0.36f, 3.36f, 4.00f, 0.16f, 0.32f); // Front base
	carBox(T, 0.0f, 1.15f, 3.51f, 1.90f, 0.32f, 0.04f); // Front grille
	carShape(carMesh.headlight, 1.70f, 1.18f, 3.50f, 0.50f, 0.20f, 0.10f); // Left headlight
	carShape(carMesh.headlight, -1.70f, 1.18f, 3.50f, 0.50f, 0.20f, 0.10f); // Right headlight
	carBox(carMesh.plate, 0.0f, 0.62f, 3.565f, 1.30f, 0.28f, 0.03f); // Front plate
	carBox(carMesh.plateBlue, -0.58f, 0.62f, 3.575f, 0.14f, 0.28f, 0.03f); // Front plate blue detail

	carBox(P, 0.0f, 0.675f, -3.375f, 4.40f, 0.55f, 0.35f); // Rear bumper
	carBox(T, 0.0f, 0.36f, -3.36f, 4.00f, 0.16f, 0.32f); // Rear base
	carBox(carMesh.taillight, 1.70f, 1.20f, -3.51f, 0.80f, 0.26f, 0.04f); // Left taillight
	carBox(carMesh.taillight, -1.70f, 1.20f, -3.51f, 0.80f, 0.26f, 0.04f); // Right taillight
	carBox(carMesh.plate, 0.0f, 0.62f, -3.565f, 1.30f, 0.28f, 0.03f); // Rear plate
	carBox(carMesh.plateBlue, 0.58f, 0.62f, -3.575f, 0.14f, 0.28f, 0.03f); // Rear plate blue detail
	carShape(carMesh.chromeCyl, -1.20f, 0.22f, -3.50f, 0.10f, 0.35f, 0.10f, 90.0f); // Exhaust pipe

	// Sides
	const float yWin = 0.5f * (CAR_BELT + CAR_ROOF), hWin = CAR_ROOF - CAR_BELT;
	for (int s = -1; s <= 1; s += 2) {
		carBox(carMesh.chrome, s * 2.27f, 1.62f, 0.25f, 0.05f, 0.08f, 0.30f); // Front door handle
		carBox(carMesh.chrome, s * 2.27f, 1.62f, -0.85f, 0.05f, 0.08f, 0.30f); // Back door handle
		carBox(T, s * 2.255f, 1.10f, -0.10f, 0.02f, 1.50f, 0.03f); // Space between doors
		carBox(T, s * 2.30f, 1.97f, 1.25f, 0.20f, 0.06f, 0.10f); // Rearview mirror support
		carBox(P, s * 2.45f, 2.02f, 1.22f, 0.36f, 0.26f, 0.20f); // Rearview mirror
		carBox(T, s * CAR_GLASS_X, yWin, CAR_WS_TOP, 0.06f, hWin, 0.05f); // Front window bar
		carBox(T, s * CAR_GLASS_X, yWin, CAR_RW_TOP, 0.06f, hWin, 0.05f); // Back window bar
	}
}

// Car interior
static void drawCarInterior(const Car& car) {
	const int I = carMesh.interior, S = carMesh.seat, T = carMesh.trim;

	carBox(I, 0.0f, 1.675f, 1.15f, 4.18f, 0.45f, 0.40f); // Dashboard
	carBox(I, 2.06f, 1.35f, 0.0f, 0.06f, 0.90f, 2.80f); // Left door
	carBox(I, -2.06f, 1.35f, 0.0f, 0.06f, 0.90f, 2.80f); // Right door
	carBox(T, 0.0f, 0.85f, 0.45f, 0.50f, 0.60f, 1.00f); // Vertical dashboard
	carShape(carMesh.chromeCyl, 0.0f, 1.25f, 0.60f, 0.06f, 0.20f, 0.06f); // Gear lever
	carBox(T, 0.0f, 2.68f, 0.50f, 0.50f, 0.14f, 0.06f); // Interior mirror
	carBox(T, 0.0f, 2.80f, 0.50f, 0.05f, 0.12f, 0.05f); // Mirror support

	// Front seats
	for (int s = -1; s <= 1; s += 2) {
		float x = s * 0.95f;
		carBox(S, x, 0.72f, 0.25f, 1.20f, 0.34f, 0.95f); // Seat cushion
		carBox(S, x, 1.35f, -0.30f, 1.20f, 1.05f, 0.22f, -12.0f); // Seat backrest
		carBox(S, x, 2.05f, -0.45f, 0.60f, 0.30f, 0.18f, -12.0f); // Seat headrest
	}

	// Back seat
	carBox(S, 0.0f, 0.72f, -0.80f, 3.90f, 0.34f, 0.55f); // Seat cushion
	carBox(S, 0.0f, 1.30f, -1.20f, 3.90f, 1.00f, 0.20f, -12.0f); // Seat backrest

	// Steering wheel
	mu.pushMatrix(gmu::MODEL);
	mu.translate(gmu::MODEL, 0.95f, 1.80f, 0.80f);
	mu.rotate(gmu::MODEL, -70.0f, 1.0f, 0.0f, 0.0f);
	mu.rotate(gmu::MODEL, car.steerVisual * 3.0f, 0.0f, 1.0f, 0.0f); // Rotate the steering wheel
	carShape(carMesh.steering, 0.0f, 0.0f, 0.0f, 0.38f, 0.38f, 0.38f);
	carBox(T, 0.0f, 0.0f, 0.0f, 0.68f, 0.04f, 0.07f);
	carBox(T, 0.0f, 0.0f, -0.17f, 0.07f, 0.04f, 0.34f);
	carShape(carMesh.trimCyl, 0.0f, -0.20f, 0.0f, 0.05f, 0.40f, 0.05f);
	mu.popMatrix(gmu::MODEL);
}

// Wheel: tire + rim + hub + 5 spokes (to see it spin)
static void drawCarWheel(const Car& car, float x, float z, bool front) {
	mu.pushMatrix(gmu::MODEL);
	mu.translate(gmu::MODEL, x, CAR_WHEEL_R - 0.2f, z); // Tire touches the ground (y = -0.2)
	if (front) {
		mu.rotate(gmu::MODEL, car.steerVisual, 0.0f, 1.0f, 0.0f); // Front wheels turn with the steering angle
	}
	mu.rotate(gmu::MODEL, car.wheelSpin, 1.0f, 0.0f, 0.0f); // Rotate the wheel

	carShape(carMesh.rubber, 0.0f, 0.0f, 0.0f, CAR_WHEEL_R, 0.35f / 0.275f, CAR_WHEEL_R, 0.0f, 0.0f, 90.0f); // pneu = toro escalado
	carShape(carMesh.trimCyl, 0.0f, 0.0f, 0.0f, 0.52f, 0.72f, 0.52f, 0.0f, 0.0f, 90.0f); // jante
	carShape(carMesh.chromeCyl, 0.0f, 0.0f, 0.0f, 0.14f, 0.76f, 0.14f, 0.0f, 0.0f, 90.0f); // cubo

	for (int k = 0; k < 5; k++) {
		mu.pushMatrix(gmu::MODEL);
		mu.rotate(gmu::MODEL, k * 72.0f, 1.0f, 0.0f, 0.0f);
		carBox(carMesh.chrome, 0.365f, 0.30f, 0.0f, 0.02f, 0.36f, 0.08f); // Outside radius of the spoke
		carBox(carMesh.chrome, -0.365f, 0.30f, 0.0f, 0.02f, 0.36f, 0.08f); // Inside radius of the spoke
		mu.popMatrix(gmu::MODEL);
	}

	mu.popMatrix(gmu::MODEL);

}

// Glass window and mirror (the alpha of the material is applied by blending, not by the shader)
static void drawCarGlass() {
	const int G = carMesh.glass, GT = carMesh.glassTri, M = carMesh.mirror;
	const float h = CAR_ROOF - CAR_BELT; // Window height
	const float yMid = 0.5f * (CAR_BELT + CAR_ROOF);
	const float aF = (CAR_WS_BASE - CAR_WS_TOP) / 1.5f;
	const float aR = (CAR_RW_TOP - CAR_RW_BASE) / 1.5f;
	float yc, zc, len, ang;

	GlassPiece list[12];
	int n = 0;

	// Windshield and rear window
	slopeParams(CAR_BELT, CAR_WS_BASE, CAR_ROOF, CAR_WS_TOP, yc, zc, len, ang);
	list[n++] = { G, true, 0.0f, yc, zc, 4.20f, len, 0.04f, ang, 0.0f, 0.0f, yc, zc, 0.0f };
	slopeParams(CAR_BELT, CAR_RW_BASE, CAR_ROOF, CAR_RW_TOP, yc, zc, len, ang);
	list[n++] = { G, true, 0.0f, yc, zc, 4.20f, len, 0.04f, ang, 0.0f, 0.0f, yc, zc, 0.0f };

	// Side windows and mirrors
	for (int s = -1; s <= 1; s += 2) {
		float x = s * CAR_GLASS_X;
		list[n++] = { G, true, x, yMid, 0.225f, 0.03f, h, 0.55f, 0.0f, 0.0f, x, yMid, 0.225f, 0.0f }; // Front window
		list[n++] = { G, true, x, yMid, -0.40f, 0.03f, h, 0.50f, 0.0f, 0.0f, x, yMid, -0.40f, 0.0f }; // Back window
		list[n++] = { GT, false, x, CAR_BELT, CAR_WS_TOP + 0.5f * aF, aF, 0.03f, h / 0.866f, 90.0f, 90.0f,
			x, CAR_BELT + h / 3.0f, (2.0f * CAR_WS_TOP + CAR_WS_BASE) / 3.0f, 0.0f }; // Front corner
		list[n++] = { GT, false, x, CAR_BELT, CAR_RW_TOP - 0.5f * aR, aR, 0.03f, h / 0.866f, -90.0f, 90.0f,
			x, CAR_BELT + h / 3.0f, (2.0f * CAR_RW_TOP + CAR_RW_BASE) / 3.0f, 0.0f }; // Back corner
		list[n++] = { M, true, s * 2.45f, 2.02f, 1.113f, 0.30f, 0.20f, 0.02f, 0.0f, 0.0f,
			s * 2.45f, 2.02f, 1.113f, 0.0f }; // Side mirror
	}

	// Distance from each piece to the camera: -z in eye coordinates (VIEW * MODEL of the car)
	mu.computeDerivedMatrix(gmu::VIEW_MODEL);
	float* vm = mu.get(gmu::VIEW_MODEL);
	for (int i = 0; i < n; i++)
		list[i].depth = -(vm[2] * list[i].cx + vm[6] * list[i].cy + vm[10] * list[i].cz + vm[14]);

	// Sort the pieces from back to front (descending depth)
	sort(list, list + n, [](const GlassPiece& a, const GlassPiece& b) { return a.depth > b.depth; });

	for (int i = 0; i < n; i++) {
		const GlassPiece& p = list[i];
		glBlendColor(0.0f, 0.0f, 0.0f, renderer.myMeshes[p.mesh].mat.diffuse[3]); // Opacity
		if (p.box)
			carBox(p.mesh, p.x, p.y, p.z, p.sx, p.sy, p.sz, p.rotX);
		else
			carShape(p.mesh, p.x, p.y, p.z, p.sx, p.sy, p.sz, p.rotX, 0.0f, p.rotZ);
	}
}

// Plate text (Truetype)
static void drawPlateText(float cx, float cy, float cz, bool back) {
	if (!fontLoaded) return;

	const std::string plateText = "AVTM-G5";
	const float targetWidth = 1.00f; // Width of the text on the plate
	float widthPx = renderer.textWidth(plateText); // Font width in pixels
	float s = targetWidth / widthPx; // Pixels of the font
	float capPx = 0.64f * 128.0f; // Uppercase height (Arial, atlas a 128 px)

	mu.pushMatrix(gmu::MODEL);
	mu.translate(gmu::MODEL, cx, cy, cz);
	if (back)
		mu.rotate(gmu::MODEL, 180.0f, 0.0f, 1.0f, 0.0f);
	mu.scale(gmu::MODEL, s, s, s);
	mu.translate(gmu::MODEL, -0.5f * widthPx, -0.5f * capPx, 0.0f); // Centers
	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);

	TextCommand t;
	t.str = plateText;
	t.position[0] = 0.0f;
	t.position[1] = 0.0f;
	t.size = 1.0f;
	t.color[0] = 0.05f; t.color[1] = 0.05f; t.color[2] = 0.08f; t.color[3] = 1.0f;
	t.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
	renderer.renderText(t);

	mu.popMatrix(gmu::MODEL);
}

void drawCar(const Car& car) {
	mu.pushMatrix(gmu::MODEL);
	mu.translate(gmu::MODEL, car.x, car.y, car.z);
	mu.rotate(gmu::MODEL, car.angle, 0.0f, 1.0f, 0.0f);
	mu.rotate(gmu::MODEL, car.fallPitch, 1.0f, 0.0f, 0.0f); // cair
	mu.rotate(gmu::MODEL, car.fallRoll, 0.0f, 0.0f, 1.0f);

	// Opaque parts
	drawCarBody();
	drawCarInterior(car);
	drawCarWheel(car, 1.92f, 2.35f, true); // Left front
	drawCarWheel(car, -1.92f, 2.35f, true); // Right front
	drawCarWheel(car, 1.92f, -2.35f, false); // Left back
	drawCarWheel(car, -1.92f, -2.35f, false); // Right back

	// Transparent parts
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);

	// Windows: constant alpha per piece (glBlendColor), same for all windows
	glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
	drawCarGlass();

	// Text: uses the alpha of each letter (transparent background of the font)
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	drawPlateText(0.07f, 0.62f, 3.60f, false); // Front plate
	drawPlateText(-0.07f, 0.62f, -3.60f, true); // Back plate
	renderer.activateRenderMeshesShaderProg(); // Render

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	mu.popMatrix(gmu::MODEL);
}

// ============================================================================
// ENVIRONMENT OBJECTS & GAMEPLAY ENTITIES
// ============================================================================

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

void drawOrange(const Orange& orange)
{
	mu.pushMatrix(gmu::MODEL);

	mu.translate(gmu::MODEL, orange.x, 1.0f, orange.z);

	// Movement in X -> rotation in Z
	if (orange.dirX != 0.0f)
		mu.rotate(gmu::MODEL, -orange.angle * orange.dirX,
			0.0f, 0.0f, 1.0f);

	// Movement in Z -> rotation in X
	if (orange.dirZ != 0.0f)
		mu.rotate(gmu::MODEL, orange.angle * orange.dirZ,
			1.0f, 0.0f, 0.0f);

	// Orange
	drawCenteredObject(
		ORANGE_MESH,
		0.0f, 0.0f, 0.0f,
		1.5f, 1.5f, 1.5f,
		0.0f, 0.0f, 0.0f,
		0
	);

	// Black mesh
	drawCenteredObject(
		ORANGE_BLACK_MESH,
		0.0f, 1.45f, 0.0f,
		0.18f, 0.18f, 0.18f,
		0.0f, 0.0f, 0.0f,
		0
	);

	mu.popMatrix(gmu::MODEL);
}

bool orangePathIsFree(const Orange& orange)
{
	// Safety margin
	const float margin = 4.5f;

	// Check butters
	for (int i = 0; i < NUM_BUTTERS; i++)
	{
		// Move horizontally (in X)
		if (orange.dirX != 0.0f)
		{
			if (fabs(orange.z - butters[i].z) < margin)
				return false;
		}

		// Move vertically (in Z)
		else if (orange.dirZ != 0.0f)
		{
			if (fabs(orange.x - butters[i].x) < margin)
				return false;
		}
	}

	// Candles positions (x, z)
	float candles[6][2] = {
		{ 51.25f,  32.25f },
		{ -5.5f,   48.5f },
		{-45.5f,   50.5f },
		{-35.5f,  -50.5f },
		{ 20.5f,  -19.0f },
		{ 55.5f,  -41.0f }
	};

	// Check candles
	for (int i = 0; i < 6; i++)
	{
		if (orange.dirX != 0.0f)
		{
			if (fabs(orange.z - candles[i][1]) < margin)
				return false;
		}

		if (orange.dirZ != 0.0f)
		{
			if (fabs(orange.x - candles[i][0]) < margin)
				return false;
		}
	}

	return true;
}

void resetOrange(Orange& orange)
{
	do
	{
		int side = rand() % 4;

		if (side == 0)
		{
			// Left -> right
			orange.x = -87.0f;
			orange.z = -80.0f + (rand() % 161);

			orange.dirX = 1.0f;
			orange.dirZ = 0.0f;
		}
		else if (side == 1)
		{
			// Right -> left
			orange.x = 87.0f;
			orange.z = -80.0f + (rand() % 161);

			orange.dirX = -1.0f;
			orange.dirZ = 0.0f;
		}
		else if (side == 2)
		{
			// Top -> bottom
			orange.x = -80.0f + (rand() % 161);
			orange.z = 87.0f;

			orange.dirX = 0.0f;
			orange.dirZ = -1.0f;
		}
		else
		{
			// Bottom -> top
			orange.x = -80.0f + (rand() % 161);
			orange.z = -87.0f;

			orange.dirX = 0.0f;
			orange.dirZ = 1.0f;
		}

	} while (!orangePathIsFree(orange));

	orange.angle = 0.0f;
}

void updateOranges()
{
	for (int i = 0; i < NUM_ORANGES; i++)
	{
		// Speed up the orange over time
		oranges[i].speed += oranges[i].acceleration;

		// Update the position of the orange based on its direction and speed
		oranges[i].x += oranges[i].dirX * oranges[i].speed;
		oranges[i].z += oranges[i].dirZ * oranges[i].speed;

		// Rotate the orange based on its speed (for visual effect)
		oranges[i].angle += oranges[i].speed * 10.0f;

		if (oranges[i].angle >= 360.0f)
			oranges[i].angle -= 360.0f;

		// If the orange goes out of bounds, reset its position
		if (oranges[i].x > 87.0f || oranges[i].x < -87.0f ||
			oranges[i].z > 87.0f || oranges[i].z < -87.0f)
		{
			resetOrange(oranges[i]);
		}
	}
}

void initCheerios() {
	cheerioInstances.clear();
	float spacing = 6.0f;

	for (int i = 0; i < NUM_CHEERIOS_LINES; i++) {
		float x1 = cheeriosLine[i].x1;
		float z1 = cheeriosLine[i].z1;
		float x2 = cheeriosLine[i].x2;
		float z2 = cheeriosLine[i].z2;

		float dx = x2 - x1;
		float dz = z2 - z1;
		float dist = sqrtf(dx * dx + dz * dz);  // Calculate the distance between the two points
		int count = (int)(dist / spacing); // Calculate the number of cheerios to draw based on the distance and spacing

		for (int j = 0; j <= count; ++j) {
			float t = (count == 0) ? 0.0f : (float)j / (float)count; // Calculate the interpolation factor (how far along the line we are | t = 0 to 1)
			cheerioInstances.push_back({ x1 + t * dx, z1 + t * dz });
		}
	}
}

// ============================================================================
// GAME LOGIC & CAR DYNAMICS
// ============================================================================

void startCarFall() {
	if (carFalling) {
		return;
	}

	carFalling = true;
	fallTimer = 0.0f;
	fallVelocityY = 0.0f;

	carBarbie.fallPitch = 0.0f;
	carBarbie.fallRoll = 0.0f;

	// Guardar direção que o carro tinha quando saiu da mesa
	float angleRad = carBarbie.angle * 3.14159265f / 180.0f;
	fallDirX = sin(angleRad);
	fallDirZ = cos(angleRad);

	// Mantém a velocidade que tinha ao sair
	fallHorizontalSpeed = carBarbie.speed;

	// Já não dá para andar com o carro
	keyFrente = false;
	keyTras = false;
	keyDir = false;
	keyEsq = false;

}

void respawnCar() {
	// re inicializar tudo
	carBarbie.x = CAR_START_X;
	carBarbie.z = CAR_START_Z;
	carBarbie.angle = CAR_START_ANGLE;

	carBarbie.speed = 0.0f;
	carBarbie.fallPitch = 0.0f;
	carBarbie.fallRoll = 0.0f;

	fallVelocityY = 0.0f;
	fallHorizontalSpeed = 0.0f;
	fallTimer = 0.0f;
	carFalling = false;

	keyFrente = false;
	keyTras = false;
	keyDir = false;
	keyEsq = false;
}

void restartGame()
{
	respawnCar();
	lives = 5;
	points = 0;

	for (int i = 0; i < NUM_ORANGES; i++)
	{
		resetOrange(oranges[i]);
	}

	paused = false;
}

void updateCarFall(float deltaTime) {
	fallTimer += deltaTime;

	// Gravity
	fallVelocityY -= FALL_GRAVITY * deltaTime;
	carBarbie.y += fallVelocityY * deltaTime;

	// Preserve momentum trajectory
	carBarbie.x += fallDirX * fallHorizontalSpeed * deltaTime;
	carBarbie.z += fallDirZ * fallHorizontalSpeed * deltaTime;

	// Lose horizontal speed over time (air resistance)
	fallHorizontalSpeed *= (1.0f - 0.8f * deltaTime);

	// Kirby fall
	float speedFactor = std::min(fabs(fallHorizontalSpeed) / carBarbie.maxSpeed, 1.0f);
	carBarbie.fallPitch += (220.0f + 180.0f * speedFactor) * deltaTime;
	carBarbie.fallRoll += (100.0f + 120.0f * speedFactor) * deltaTime;

	if (carBarbie.fallPitch >= 360.0f) {
		carBarbie.fallPitch -= 360.0f;
	}

	if (carBarbie.fallRoll >= 360.0f) {
		carBarbie.fallRoll -= 360.0f;
	}

	// After a certain time, respawn the car
	if (fallTimer >= FALL_RESPAWN_TIME) {
		respawnCar();
	}

}

void updateCarMoviment(float tableWidth, float tableDepth, float deltaTime) {
	const float deceleration = 20.0f;
	const float brakePower = 70.0f;
	const float turnSpeed = 160.0f;

	// Acceleration and braking controls
	// W and S at the same time: brake
	if (keyFrente && keyTras) {
		if (carBarbie.speed > 0.0f) {
			carBarbie.speed -= brakePower * deltaTime;

			if (carBarbie.speed < 0.0f) {
				carBarbie.speed = 0.0f;
			}
				
		}

		else if (carBarbie.speed < 0.0f) {
			carBarbie.speed += brakePower * deltaTime;

			if (carBarbie.speed > 0.0f) {
				carBarbie.speed = 0.0f;
			}
		}

	}

	else if (keyFrente) { // W
		// If the car is moving backward: brake
		if (carBarbie.speed < 0.0f) {
			carBarbie.speed += brakePower * deltaTime;
			if (carBarbie.speed > 0.0f) {
				carBarbie.speed = 0.0f;
			}	
		}

		// If the car is stopped or moving forward: accelerate
		else {
			carBarbie.speed += carBarbie.acceleration * deltaTime;
		}

	}

	else if (keyTras) { // S

		// If the car is moving forward: brake
		if (carBarbie.speed > 0.0f) {
			carBarbie.speed -= brakePower * deltaTime;

			if (carBarbie.speed < 0.0f) {
				carBarbie.speed = 0.0f;
			}
				
		}

		// If the car is stopped or moving backward: accelerate backward
		else {
			carBarbie.speed -= carBarbie.acceleration * deltaTime;
		}

	}

	// No keys pressed: decelerate to a stop
	else {
		if (carBarbie.speed > 0.0f) {
			carBarbie.speed -= deceleration * deltaTime;

			if (carBarbie.speed < 0.0f) {
				carBarbie.speed = 0.0f;
			}
				
		}

		else if (carBarbie.speed < 0.0f) {
			carBarbie.speed += deceleration * deltaTime;

			if (carBarbie.speed > 0.0f) {
				carBarbie.speed = 0.0f;
			}
				
		}

	}

	// Maximum speed limit
	if (carBarbie.speed > carBarbie.maxSpeed) {
		carBarbie.speed = carBarbie.maxSpeed;
	}
		
	if (carBarbie.speed < -carBarbie.maxSpeed) {
		carBarbie.speed = -carBarbie.maxSpeed;
	}
	

	// Turning controls
	float turnDirection = 0.0f;

	// A: left
	if (keyEsq && !keyDir) {
		turnDirection = 1.0f;
	}
		
	// D: right
	else if (keyDir && !keyEsq) {
		turnDirection = -1.0f;
	}
		
	// Only turn if the car is moving (forward or backward)
	if (fabs(carBarbie.speed) > 0.01f) {
		float speedFactor = fabs(carBarbie.speed) / carBarbie.maxSpeed;
		if (speedFactor > 1.0f) {
			speedFactor = 1.0f;
		}

		// Backward movement: invert the turning direction
		float movementDirection = (carBarbie.speed >= 0.0f) ? 1.0f : -1.0f;

		// Change car direction faster when moving faster
		carBarbie.angle += turnDirection * turnSpeed * speedFactor * deltaTime * movementDirection;
	}

	// Maximum angle
	if (carBarbie.angle >= 360.0f){
		carBarbie.angle -= 360.0f;
	}
		
	if (carBarbie.angle < 0.0f){
		carBarbie.angle += 360.0f;
	}

	// Movement direction: 3D unit vector (speed and acceleration are scalars)
	float angleRad = carBarbie.angle * 3.14159265f / 180.0f;
	carBarbie.dir[0] = sinf(angleRad);
	carBarbie.dir[1] = 0.0f;
	carBarbie.dir[2] = cosf(angleRad);

	// Move
	carBarbie.x += carBarbie.dir[0] * carBarbie.speed * deltaTime;
	carBarbie.z += carBarbie.dir[2] * carBarbie.speed * deltaTime;
	carBarbie.wheelSpin += (carBarbie.speed * deltaTime / CAR_WHEEL_R) * 180.0f / 3.14159265f;
	carBarbie.wheelSpin = fmodf(carBarbie.wheelSpin, 360.0f);
	float steerTarget = turnDirection * 20.0f;
	carBarbie.steerVisual += (steerTarget - carBarbie.steerVisual) * std::min(1.0f, 10.0f * deltaTime);

	// Check if the car has fallen off the table by comparing its center position with the table boundaries
	if (fabs(carBarbie.x) > (tableWidth * 0.5f) || fabs(carBarbie.z) > (tableDepth * 0.5f)) {
		startCarFall();
	}
		
}

// ============================================================================
// COLLISION DETECTION SYSTEM
// ============================================================================

// Get the AABB for the car, butter, orange and cheerio
AABB getCarAABB(const Car& car) {
	float halfW = car.width * 0.5f - 0.1f;
	float halfD = car.width * 0.5f - 0.1f;
	return { car.x - halfW, car.x + halfW, car.z - halfD, car.z + halfD };
}

AABB getButterAABB(const Butter& butter) {
	float halfW = 2.35f;
	float halfD = 1.0f;
	return { butter.x - halfW, butter.x + halfW, butter.z - halfD, butter.z + halfD };
}

AABB getOrangeAABB(const Orange& orange) {
	float radius = 1.5f;
	return { orange.x - radius, orange.x + radius, orange.z - radius, orange.z + radius };
}

AABB getCheerioAABB(const CheerioInstance& cheerio) {
	float halfSize = 0.6f;
	return { cheerio.x - halfSize, cheerio.x + halfSize, cheerio.z - halfSize, cheerio.z + halfSize };
}

// Axis-Aligned Bounding Box (AABB) collision detection
bool checkAABBCollision(const AABB& a, const AABB& b) {
	return (a.minX <= b.maxX && a.maxX >= b.minX) && // horizontal overlap, x
		(a.minZ <= b.maxZ && a.maxZ >= b.minZ); // vertical overlap, z
}

// Check for collisions between the car and other objects
void checkCollisions() {
	AABB carBox = getCarAABB(carBarbie);

	// 1. Verify collision with Oranges (Car loses a life and respawns)
	for (int i = 0; i < NUM_ORANGES; i++) {
		AABB orangeBox = getOrangeAABB(oranges[i]);
		if (checkAABBCollision(carBox, orangeBox)) {
			lives--;
			respawnCar();
			if (lives <= 0) {
				paused = true;
			}
			return;
		}
	}

	// 2. Verify collision with Butters (Car stops and pushes the butter)
	for (int i = 0; i < NUM_BUTTERS; i++) {
		AABB butterBox = getButterAABB(butters[i]);
		if (checkAABBCollision(carBox, butterBox)) {
			// Push the butter slightly in the direction of the car's movement
			float pushDist = 0.4f;
			butters[i].x += carBarbie.dir[0] * pushDist;
			butters[i].z += carBarbie.dir[2] * pushDist;

			// Stop the car
			carBarbie.speed = 0.0f;
		}
	}

	// 3. Verify collision with Cheerios (Car stops and pushes the cheerio)
	for (auto& cheerio : cheerioInstances) {
		AABB cheerioBox = getCheerioAABB(cheerio);
		if (checkAABBCollision(carBox, cheerioBox)) {
			// Push the cheerio slightly in the direction of the car's movement
			float pushDist = 0.5f;
			cheerio.x += carBarbie.dir[0] * pushDist;
			cheerio.z += carBarbie.dir[2] * pushDist;

			// Stop the car
			carBarbie.speed = 0.0f;
		}
	}
}

// ============================================================================
// CAMERA CONTROLS & HUD RENDERING
// ============================================================================

// Setup the camera based on the current camera mode and table dimensions
void setupCamera(float tableWidth, float tableDepth, float tablePosY) {
	mu.loadIdentity(gmu::VIEW);
	mu.loadIdentity(gmu::PROJECTION);

	// Camera margin: 10% extra space around the table
	float margin = 1.10f;

	// Camera depends on the table dimensions and aspect ratio
	float halfView = std::max(tableDepth * 0.5f, (tableWidth * 0.5f) / aspectRatio);
	halfView *= margin;

	// Camera 1: Top-down orthographic
	if (CameraMode == 1) {
		mu.ortho(-halfView * aspectRatio, halfView * aspectRatio, -halfView, halfView, 0.1f, 1000.0f);

		mu.lookAt(
			0.0f, 200.0f,    0.0f,	// Position of the camera (above the table)
			0.0f, tablePosY, 0.0f,	// Look at the center of the table
			0.0f, 0.0f,     -1.0f	// Top of the camera is in the negative Z direction
		);
	}

	// Camera 2: Fixed top perspective
	else if (CameraMode == 2) {


		mu.perspective(53.13f, aspectRatio, 0.1f, 1000.0f);

		mu.lookAt(
			0.0f, tablePosY + (1.4f * halfView), (1.8f * halfView),	// Position of the camera (above and in front of the table)
			0.0f, tablePosY,                     0.0f,				// Look at the center of the table
			0.0f, 0.0f,                         -1.0f				// Top of the image
		);
	}

	// Camera 3: Third-person follow perspective (accept mouse movement)
	else if (CameraMode == 3) {
		mu.perspective(53.13f, aspectRatio, 0.1f, 1000.0f);

		// Camera orientation follows the mouse orientation
		float carAngleRad = carBarbie.angle * 3.14f / 180.0f;

		float offsetX = camX * cos(carAngleRad) + camZ * sin(carAngleRad);
		float offsetZ = -camX * sin(carAngleRad) + camZ * cos(carAngleRad);

		// Final camera position
		float cameraX = carBarbie.x + offsetX;
		float cameraY = carBarbie.y + 1.5f + camY;
		float cameraZ = carBarbie.z + offsetZ;

		// Camera looks at the car's position
		mu.lookAt(cameraX, cameraY, cameraZ,
			carBarbie.x, carBarbie.y + 1.5f, carBarbie.z,
			0.0f, 1.0f, 0.0f);
	}
}

void drawHUD()
{
	if (!fontLoaded)
		return;

	// Texto deve aparecer à frente de toda a cena
	glDisable(GL_DEPTH_TEST);

	// O fundo dos glyphs é transparente
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Obter dimensões reais do viewport
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Preparar matrizes para coordenadas do ecrã
	mu.pushMatrix(gmu::MODEL);
	mu.loadIdentity(gmu::MODEL);

	mu.pushMatrix(gmu::VIEW);
	mu.loadIdentity(gmu::VIEW);

	mu.pushMatrix(gmu::PROJECTION);
	mu.loadIdentity(gmu::PROJECTION);

	mu.ortho(
		viewport[0],
		viewport[0] + viewport[2] - 1,
		viewport[1],
		viewport[1] + viewport[3] - 1,
		-1.0f,
		1.0f
	);

	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);

	// LIVES
	TextCommand livesText;
	livesText.str = "Lives: " + std::to_string(lives);
	livesText.position[0] = 20.0f;
	livesText.position[1] = WinY - 40.0f;
	livesText.size = 0.25f;

	livesText.color[0] = 1.0f;
	livesText.color[1] = 1.0f;
	livesText.color[2] = 1.0f;
	livesText.color[3] = 1.0f;

	livesText.pvm = mu.get(gmu::PROJ_VIEW_MODEL);

	renderer.renderText(livesText);


	// POINTS
	TextCommand pointsText;
	pointsText.str = "Points: " + std::to_string(points);
	pointsText.position[0] = 20.0f;
	pointsText.position[1] = WinY - 70.0f;
	pointsText.size = 0.25f;

	pointsText.color[0] = 1.0f;
	pointsText.color[1] = 1.0f;
	pointsText.color[2] = 1.0f;
	pointsText.color[3] = 1.0f;

	pointsText.pvm = mu.get(gmu::PROJ_VIEW_MODEL);

	renderer.renderText(pointsText);

	// PAUSED
	if (paused)
	{
		TextCommand pauseText;
		pauseText.str = "PAUSED";
		pauseText.size = 1.0f;

		pauseText.position[0] = WinX / 2.0f - renderer.textWidth(pauseText.str) * pauseText.size / 2.0f;
		pauseText.position[1] = WinY - 80;

		pauseText.color[0] = 1.0f;
		pauseText.color[1] = 1.0f;
		pauseText.color[2] = 1.0f;
		pauseText.color[3] = 1.0f;

		pauseText.pvm = mu.get(gmu::PROJ_VIEW_MODEL);

		renderer.renderText(pauseText);

		TextCommand restartText;
		restartText.str = "Press R to restart";
		restartText.size = 0.25f;

		restartText.position[0] = WinX / 2.0f - renderer.textWidth(restartText.str) * restartText.size / 2.0f;
		restartText.position[1] = WinY - 100.0f;

		restartText.color[0] = 1.0f;
		restartText.color[1] = 1.0f;
		restartText.color[2] = 1.0f;
		restartText.color[3] = 1.0f;

		restartText.pvm = mu.get(gmu::PROJ_VIEW_MODEL);

		renderer.renderText(restartText);
	}

	// Restaurar matrizes
	mu.popMatrix(gmu::PROJECTION);
	mu.popMatrix(gmu::VIEW);
	mu.popMatrix(gmu::MODEL);

	// Restaurar estado OpenGL
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	// renderText mudou o shader ativo
	renderer.activateRenderMeshesShaderProg();
}

// ============================================================================
// MAIN SIMULATION LOOP
// ============================================================================

void renderSim(void) {

	FrameCount++;

	// Geometry parameters to scale and translate the objects in the scene
	float tableWidth = 175.0f, tableHeight = 1.0f, tableDepth = 175.0f;
	float tablePosY = -1.0f;

	float roadWidth = 20.0f, roadHeight = 0.1f;
	float roadPosY = tablePosY * 0.5f;

	float marginWidth = 1.0f, marginHeight = 1.0f;
	float marginPosY = roadPosY + 0.3f;

	float cheerioPosY = marginPosY + 1.0f;
	float spacing = 6.0f;

	float candleBasePosY = roadPosY + 6.0f;
	float candleWickPosY = roadPosY + 12.0f + 1.5f;

	if (!carFalling) {
		carBarbie.y = roadPosY + roadHeight * 0.5f + 0.2f;
	}

	// Delta time
	static int previousTime = glutGet(GLUT_ELAPSED_TIME);
	int currentTime = glutGet(GLUT_ELAPSED_TIME);
	float deltaTime = (currentTime - previousTime) / 1000.0f;
	previousTime = currentTime;
	// Avoid jumps if the window is paused, dragged, debugger, etc.
	if (deltaTime > 0.05f) {
		deltaTime = 0.05f;
	}
	if (!paused)
	{
		if (carFalling) {
			updateCarFall(deltaTime);
		}
		else {
			updateCarMoviment(tableWidth, tableDepth, deltaTime);
			checkCollisions();
		}

		updateOranges();
	}

	// Clear the color and depth buffers to prepare for rendering the new frame
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Activate the shader program for rendering meshes with illumination
	renderer.activateRenderMeshesShaderProg();
	// Set the texture units for the shader program
	renderer.setTexUnit(0, 0); // stone.tga
	renderer.setTexUnit(1, 1); // checker.png
	renderer.setTexUnit(2, 2); // lightwood.tga
	renderer.setTexUnit(3, 3); // road.jpg

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

	// Draw the cheerios along the road margins
	for (const auto& cheerio : cheerioInstances) {
		drawObject(CHEERIO_MESH, cheerio.x, cheerioPosY, cheerio.z, 1.0f, 1.0f, 1.0f);
	}

	// Draw the start flag and start line
	drawObject(ROAD_MESH,   60.0f, roadPosY + 7.5f,  -5.0f, 1.0f,      15.0f, 1.0f); // 1: Left flag pole
	drawObject(ROAD_MESH,   80.0f, roadPosY + 7.5f,  -5.0f, 1.0f,      15.0f, 1.0f); // 2: Right flag pole
	drawObject(MARGIN_MESH, 70.0f, roadPosY + 10.0f, -5.0f, roadWidth, 3.0f,  1.0f); // 3: Flag
	drawObject(MARGIN_MESH, 70.0f, marginPosY,       -5.0f, roadWidth, 0.1f,  1.0f); // 4: Start line

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

	// Draw the butters
	for (int i = 0; i < NUM_BUTTERS; i++) {
		drawButter(butters[i]);
	}

	// Draw the oranges
	for (int i = 0; i < NUM_ORANGES; i++) {
		drawOrange(oranges[i]);
	}

	// Draw the car
	drawCar(carBarbie);
	drawHUD();
	glutSwapBuffers();
}

// ============================================================================
// GLUT CALLBACKS & USER INPUT HANDLERS
// ============================================================================

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

// Callback function for window resizing
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

		case 'm':    //reset
		case 'M':
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

		// iniciar o movimento ou aceleração
		case 'w':
		case 'W':
			keyFrente = true;
			break;

		case 'a':
		case 'A':
			keyEsq = true;
			break;

		case 's':
		case 'S':
			keyTras = true;
			break;

		case 'd':
		case 'D':
			keyDir = true;
			break;

		case 'p':
		case 'P':
			paused = !paused;
			break;

		case 'r':
		case 'R':
			if (paused)
			{
				restartGame();
				printf("Game restarted\n");
			}
			break;
	}
}

void processKeyUp(unsigned char key, int xx, int yy)
{
	switch (key) {

	// parar o movimento ou aceleração
	case 'w':
	case 'W':
		keyFrente = false;
		break;

	case 'a':
	case 'A':
		keyEsq = false;
		break;

	case 's':
	case 'S':
		keyTras = false;
		break;

	case 'd':
	case 'D':
		keyDir = false;
		break;
	}
}

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

// ============================================================================
// SCENE SETUP & MAIN
// ============================================================================

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

	// create geometry and VAO for the butter (11)
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

	// create geometry and VAO for the butter - beige part of the package (12)
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

	// create geometry and VAO for the butter - blue part of the package (13)
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

	// create geometry and VAO for the orange (14)
	float ambOrange[] = { 0.30f, 0.10f, 0.00f, 1.0f };
	float diffOrange[] = { 1.00f, 0.35f, 0.02f, 1.0f };
	float specOrange[] = { 0.30f, 0.30f, 0.20f, 1.0f };
	amesh = createSphere(1.0f, 20);
	memcpy(amesh.mat.ambient, ambOrange, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffOrange, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specOrange, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = 30.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO for the black part of the orange (15)
	float ambBlack[] = { 0.01f, 0.01f, 0.01f, 1.0f };
	float diffBlack[] = { 0.02f, 0.02f, 0.02f, 1.0f };
	float specBlack[] = { 0.10f, 0.10f, 0.10f, 1.0f };
	amesh = createSphere(1.0f, 20);
	memcpy(amesh.mat.ambient, ambBlack, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffBlack, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specBlack, 4 * sizeof(float));
	amesh.mat.shininess = 20.0f;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO for the cheerio (16)
	float ambCheerio[] = { 0.35f, 0.30f, 0.05f, 1.0f };
	float diffCheerio[] = { 0.95f, 0.85f, 0.10f, 1.0f };
	float specCheerio[] = { 0.50f, 0.50f, 0.20f, 1.0f };
	amesh = createTorus(1.0f, 2.0f, 20, 20);
	memcpy(amesh.mat.ambient, ambCheerio, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diffCheerio, 4 * sizeof(float));
	memcpy(amesh.mat.specular, specCheerio, 4 * sizeof(float));
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

	buildCarMeshes();


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

int main(int argc, char **argv) {

	srand((unsigned int)time(NULL));

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
	//glutIdleFunc(renderSim);  // Use it for maximum performance
	glutTimerFunc(0, refresh, 0);    //use it to to get 60 FPS whatever

//	Mouse and Keyboard Callbacks
	glutKeyboardFunc(processKeys);
	glutKeyboardUpFunc(processKeyUp);
	glutIgnoreKeyRepeat(1); // ignora o repeat key do windows
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
	initCheerios();

	if(!renderer.setRenderMeshesShaderProg("shaders/mesh.vert", "shaders/mesh.frag") || 
		!renderer.setRenderTextShaderProg("shaders/ttf.vert", "shaders/ttf.frag"))
	return(1);

	//  GLUT main loop
	glutMainLoop();


	return(0);
}
