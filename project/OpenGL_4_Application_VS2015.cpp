#define GLEW_STATIC

#include "stdafx.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "GLEW/glew.h"
#include "GLFW/glfw3.h"
#include <string>
#include "Shader.hpp"
#include "Camera.hpp"
#define TINYOBJLOADER_IMPLEMENTATION

#include "Model3D.hpp"
#include "Mesh.hpp"
#include "SkyBox.hpp"

extern "C" {
	_declspec(dllexport) int NvOptimusEnablement = 0x00000001;
}

int glWindowWidth = 1920;//1800;
int glWindowHeight = 1200;//1000;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

const GLuint SHADOW_WIDTH = 8192, SHADOW_HEIGHT = 8192;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat3 lightDirMatrix;
GLuint lightDirMatrixLoc;

glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightPos1;
GLuint lightPos1Loc;
glm::vec3 lightPos2;
GLuint lightPos2Loc;
glm::vec3 lightColor;
GLuint lightColorLoc;
glm::vec3 lightDir1;
GLuint lightDirLoc1;
glm::vec3 lightColor1;
GLuint lightColorLoc1;
GLuint lightPos3Loc;
GLuint lightPos4Loc;
GLuint lightPos5Loc;
GLuint transparencyLoc;

glm::vec3 lightPos3;
glm::vec3 lightPos4;
glm::vec3 lightPos5;

gps::Camera myCamera(glm::vec3(13.667298, 1.075181, -0.966389), glm::vec3(2.0f, 1.0f, 0.0f));
GLfloat cameraSpeed = 0.2f;
bool pressedKeys[1024];
GLfloat angle;
GLfloat lightAngle;

gps::Model3D house;
gps::Model3D scene;
gps::Model3D bigPlane;
gps::Model3D deer;
gps::Model3D water;
gps::Model3D trees;
gps::Model3D test;
gps::Model3D deerr;
gps::Model3D tail;
gps::Model3D skydom;
gps::Model3D swan;

gps::SkyBox mySkyBoxDay;
gps::SkyBox mySkyBoxNight;
gps::Shader skyboxShader;

glm::mat4 deerModel = glm::mat4(1.0f);

gps::Shader myCustomShader;
gps::Shader lightShader;
gps::Shader depthMapShader;

bool animationStop = false;
gps::Model3D objects[3];
bool day = true;

GLuint shadowMapFBO;
GLuint depthMapTexture;

using namespace std;

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height)
{
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	//TODO
	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	myCustomShader.useShaderProgram();

	//set projection matrix
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	//send matrix data to shader
	GLint projLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	
	lightShader.useShaderProgram();
	
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	//set Viewport transform
	glViewport(0, 0, retina_width, retina_height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

bool firstMouse = true;

float lastX = 400, lastY = 300;
float yaw = -90.0f, pitch;

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.05f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	printf("PITCH %f and YAW %f \n", pitch, yaw);
	myCamera.rotate(pitch, yaw);
}

GLfloat cameraSpeedAnimation = 0.05f;
bool cameraAnimation = false;

bool collision(glm::vec3 camPos, gps::Model3D object)
{
	return(camPos.x >= object.minX && camPos.x <= object.maxX && camPos.y >= object.minY && camPos.y <= object.maxY && camPos.z >= object.minZ && camPos.z <= object.maxZ);
}

bool tryCollision(glm::vec3 camPos)
{
	for (int i = 0; i < 3; i++)
	{
		if (collision(camPos, objects[i]))
		{
			return true;
		}
	}

	return false;
}

bool transparency = false;
float transparencyLevel;
void processMovement()
{
	if (pressedKeys[GLFW_KEY_T])
	{
		transparency = !transparency;
		if (transparency)
		{
			transparencyLevel = 0.3;
		}
		else
		{
			transparencyLevel = 1;
		}

		transparencyLoc = glGetUniformLocation(myCustomShader.shaderProgram, "transparency");
		glUniform1f(transparencyLoc, transparencyLevel);
	}

	if (pressedKeys[GLFW_KEY_1])
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	if (pressedKeys[GLFW_KEY_2])
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (pressedKeys[GLFW_KEY_3])
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}

	if (pressedKeys[GLFW_KEY_Q]) {
		angle += 1.0f;
		if (angle > 360.0f)
			angle -= 360.0f;
	}

	if (pressedKeys[GLFW_KEY_E]) {
		angle -= 1.0f;
		if (angle < 0.0f)
			angle += 360.0f;
	}

	if (pressedKeys[GLFW_KEY_W]) {
		if (!tryCollision(myCamera.getCameraPosition() + myCamera.getCameraDirection() * cameraSpeed))
		{
			myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		}
	}

	if (pressedKeys[GLFW_KEY_S]) {
		if (!tryCollision(myCamera.getCameraPosition() - myCamera.getCameraDirection() * cameraSpeed))
		{
			myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
		}
	}

	if (pressedKeys[GLFW_KEY_A]) {
		if (!tryCollision(myCamera.getCameraPosition() - myCamera.getCameraRightDirection() * cameraSpeed))
		{
			myCamera.move(gps::MOVE_LEFT, cameraSpeed);
		}
		
	}

	if (pressedKeys[GLFW_KEY_D]) {
		if (!tryCollision(myCamera.getCameraPosition() + myCamera.getCameraRightDirection() * cameraSpeed))
		{ 
			myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
		}	
	}

	if (pressedKeys[GLFW_KEY_J]) {

		lightAngle += 0.3f;
		if (lightAngle > 360.0f)
			lightAngle -= 360.0f;

		glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
		myCustomShader.useShaderProgram();
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
	}

	if (pressedKeys[GLFW_KEY_L]) {
		lightAngle -= 0.3f; 
		if (lightAngle < 0.0f)
			lightAngle += 360.0f;

		glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
		myCustomShader.useShaderProgram();
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
	}	

	if (pressedKeys[GLFW_KEY_I]) {
		cameraAnimation = true;
	}

	if (pressedKeys[GLFW_KEY_O]) {
		cameraAnimation = false;
	}

	if (pressedKeys[GLFW_KEY_N]) {

		day = !day;
		if (day == false)
		{
			lightColor = glm::vec3(0.0f, 0.0f, 0.0f); //night light
		}
		else
		{
			lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
		}

		lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
		glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
	}

	if (glfwGetKey(glWindow, GLFW_KEY_LEFT))
	{
		deerModel = glm::translate(deerModel, glm::vec3(-3.0, -1.1, -3));
		deerModel = glm::rotate(deerModel, -0.05f, glm::vec3(0, 1, 0));
		deerModel = glm::translate(deerModel, glm::vec3(3.0, 1.1, 3));
	}

	if (glfwGetKey(glWindow, GLFW_KEY_RIGHT))
	{
		deerModel = glm::translate(deerModel, glm::vec3(-3.0, -1.1, -3));
		deerModel = glm::rotate(deerModel, 0.05f, glm::vec3(0, 1, 0));
		deerModel = glm::translate(deerModel, glm::vec3(3.0, 1.1, 3));
	}

	if (glfwGetKey(glWindow, GLFW_KEY_UP)) {

		deerModel = glm::translate(deerModel, glm::vec3(0, 0.0, -0.05));
	}

	if (glfwGetKey(glWindow, GLFW_KEY_DOWN)) {

		deerModel = glm::translate(deerModel, glm::vec3(0, 0.0, 0.05));
	}
}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	//for Mac OS X
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_SAMPLES, 4);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwMakeContextCurrent(glWindow);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	return true;
}

void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initFBOs()
{
	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix()
{
	const GLfloat near_plane = -10.0f, far_plane = 40.0f;
	glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
	glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
	glm::mat4 lightView = glm::lookAt(myCamera.getCameraTarget() + 1.0f * lightDirTr, myCamera.getCameraTarget(), glm::vec3(0.0f, 1.0f, 0.0f));

	return lightProjection * lightView;
}

void initModels()
{
	scene = gps::Model3D("objects/scena4/sceneF.obj", "objects/scena4/");
	bigPlane = gps::Model3D("objects/scena4/planMare.obj", "objects/scena4/");
	water = gps::Model3D("objects/scena4/apa.obj", "objects/scena4/");
	objects[0] = water;
	deer = gps::Model3D("objects/scena4/test.obj", "objects/scena4/");
	deerr = gps::Model3D("objects/scena4/deer_W_tail.obj", "objects/scena4/");
	tail = gps::Model3D("objects/scena4/tail.obj", "objects/scena4/");
	trees = gps::Model3D("objects/scena4/trees4.obj", "objects/scena4/");
	swan = gps::Model3D("objects/scena4/swan2.obj", "objects/scena4/");
	objects[1] = swan;
	house = gps::Model3D("objects/scena4/house.obj", "objects/scena4/");
	objects[2] = house;
}

void initShaders()
{
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	depthMapShader.loadShader("shaders/simpleDepthMap.vert", "shaders/simpleDepthMap.frag");
}

void initUniforms()
{
	myCustomShader.useShaderProgram();

	transparencyLoc = glGetUniformLocation(myCustomShader.shaderProgram, "transparency");
	glUniform1f(transparencyLoc, 1.0f);

	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");

	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	
	lightDirMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDirMatrix");

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(2.0f, 3.0f, 2.0f);
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set the light direction (direction towards the light)
	lightPos1 = glm::vec3(-5.39f, -0.69f, 0.24f);
	lightPos1Loc = glGetUniformLocation(myCustomShader.shaderProgram, "lightPos1");
	glUniform3fv(lightPos1Loc, 1, glm::value_ptr(lightPos1));

	//set the light direction (direction towards the light)
	lightPos2 = glm::vec3(-0.39f, -0.69f, 0.24f);
	lightPos2Loc = glGetUniformLocation(myCustomShader.shaderProgram, "lightPos2");
	glUniform3fv(lightPos2Loc, 1, glm::value_ptr(lightPos2));

	lightPos3 = glm::vec3(8.217021, -0.496463, -2.172253);
	lightPos3Loc = glGetUniformLocation(myCustomShader.shaderProgram, "lightPos3");
	glUniform3fv(lightPos3Loc, 1, glm::value_ptr(lightPos3));

	lightPos4 = glm::vec3(7.13f, 0.25f, 0.24f);
	lightPos4Loc = glGetUniformLocation(myCustomShader.shaderProgram, "lightPos4");
	glUniform3fv(lightPos4Loc, 1, glm::value_ptr(lightPos4));

	
	lightPos5 = glm::vec3(3.996650, -0.228469, -5.390274);
	lightPos5Loc = glGetUniformLocation(myCustomShader.shaderProgram, "lightPos5");
	glUniform3fv(lightPos5Loc, 1, glm::value_ptr(lightPos5));
	
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	//set light color
	lightColor1 = glm::vec3(1.0f, 0.0f, 1.0f);
	lightColorLoc1 = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor1");
	glUniform3fv(lightColorLoc1, 1, glm::value_ptr(lightColor1));

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

}

float movement = -0.005f;
float rotationAngle = 0.0f;
float x = 0.0f;
float movement1 = -0.005f;
int rotateNow = 0;
float anglee = 0;
float tailRotation = 0.03;

void renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!day)
	{
		mySkyBoxNight.Draw(skyboxShader, view, projection);
	}
	else
	{
		mySkyBoxDay.Draw(skyboxShader, view, projection);
	}

//------------------------------------------------------------------FIRST PASS
	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));	
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	
	//MODEL FOR SCENE
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));

	scene.Draw(depthMapShader);

	//MODEL FOR DEER
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-5.2, -0.8, 0));
	x += movement;

	if (x < -0.9)
	{
		movement = 0.005f;
		x = -0.9;
		rotationAngle = glm::radians(180.0f);

	}
	else
	{
		if (x > 0.9)
		{
			movement = -0.005f;
			x = 0.9f;
			rotationAngle = 0;
		}
	}

	model = glm::translate(model, glm::vec3(0.0f, 0, x));
	model = glm::rotate(model, rotationAngle, glm::vec3(0, 1, 0));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));

	deer.Draw(depthMapShader);

	//HOUSE
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));

	house.Draw(depthMapShader);

	//trees
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));

	trees.Draw(depthMapShader);

	//MODEL FOR BIG PLANE
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));

	bigPlane.Draw(depthMapShader);

	//MODEL FOR DEER
	model = deerModel;
	model = glm::translate(deerModel, glm::vec3(-3.0, -1.1, -3));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));

	deerr.Draw(depthMapShader);

	anglee += tailRotation;
	if (anglee >= 0.12)
	{
		tailRotation = tailRotation * (-1);
		anglee = 0.12;
	}
	else
	{
		if (anglee <= -0.12)
		{
			tailRotation = tailRotation * (-1);
			anglee = -0.12;
		}
	}

	model = glm::translate(model, glm::vec3(-0.24, 0.58, 0.572));
	model = glm::rotate(model, anglee, glm::vec3(0, 1, 0));
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));

	tail.Draw(depthMapShader);



	glBindFramebuffer(GL_FRAMEBUFFER, 0);

//-----------------------------------------------------------------SECOND PASS
	myCustomShader.useShaderProgram();

	//send lightSpace matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));
	//send view matrix to shader
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "view"),
		1,
		GL_FALSE,
		glm::value_ptr(view));	
	//compute light direction transformation matrix
	lightDirMatrix = glm::mat3(glm::inverseTranspose(view));
	//send lightDir matrix data to shader
	glUniformMatrix3fv(lightDirMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightDirMatrix));
	glViewport(0, 0, retina_width, retina_height);
	myCustomShader.useShaderProgram();
	//bind the depth map
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);
	
	//MODEL FOR WATER
	//enable transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	model = glm::mat4(1.0f);
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	water.Draw(myCustomShader);

	glDisable(GL_BLEND);

	//MODEL FOR SCENE
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	//send model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	//create normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	scene.Draw(myCustomShader);

	//create model matrix for trees
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	//send model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	//create normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	trees.Draw(myCustomShader);

	//MODEL FOR BIG PLANE
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	//send model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	//create normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	bigPlane.Draw(myCustomShader);

	//deer animation
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-5.2, -0.8, 0));
	x += movement;

	if (x < -0.9)
	{
		movement = 0.005f;
		x = -0.9;
		rotationAngle = glm::radians(180.0f);
	
	}
	else
	{
		if (x > 0.9)
		{
			movement = -0.005f;
			x = 0.9f;
			rotationAngle = 0;
		}
	}

	model = glm::translate(model, glm::vec3(0.0f, 0, x));
	model = glm::rotate(model, rotationAngle, glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	deer.Draw(myCustomShader);

	//deer with moving tail animation
	model = deerModel;
	model = glm::translate(deerModel, glm::vec3(-3.0, -1.1, -3));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	deerr.Draw(myCustomShader);

	anglee += tailRotation;
	if (anglee >= 0.12) 
	{
		tailRotation = tailRotation * (-1);
		anglee = 0.12;
	}
	else
	{
		if (anglee <= -0.12)
		{
			tailRotation = tailRotation * (-1);
			anglee = -0.12;
		}
	}

	model = glm::translate(model, glm::vec3(-0.24, 0.58, 0.572));
	model = glm::rotate(model, anglee, glm::vec3(0, 1, 0));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	tail.Draw(myCustomShader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	swan.Draw(myCustomShader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	house.Draw(myCustomShader);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	skydom.Draw(myCustomShader);
	
	processMovement();

}

bool firstMove = false;
bool secondMove = false;
bool thirdMove = false;
bool fourthMove = false;
bool fifthMove = false;
float camAngle = -187.400055f;
double crtTime;

void showCameraAnimation()
{
	if (!firstMove )
	{	
		myCamera.setCameraPosition(13.667298, 1.075181, -0.966389);
		myCamera.rotate(-8.199998, -187.400055);
		firstMove = true;
	}
	else
	{
		if (firstMove && myCamera.getCameraPosition().z < -0.07 && !fourthMove)
		{
			myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
			secondMove = true;
		}
		else
		{
			//rotate
			if (secondMove && camAngle < 0) {
				myCamera.rotate(-8.199998, camAngle);
				camAngle += 1.0f;
				thirdMove = true;
			}
			else
			{
				if (thirdMove && myCamera.getCameraPosition().z < 0.3)
				{	
					myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
					myCamera.rotate(-21.00000, -176.650070);
					crtTime = glfwGetTime();
					fourthMove = true;
				}
				else
				{
					myCamera.setCameraTarget(0, 1, 0);
					if (fourthMove && glfwGetTime() - crtTime <= 10)
					{
						thirdMove = false;
						myCamera.setCameraDirection(glm::normalize(glm::vec3(4.143175, 1.494056, -0.989048) - myCamera.getCameraPosition()));
						myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
					}
				}
			}
		}
	}
}

int main(int argc, const char * argv[]) {

	initOpenGLWindow();
	initOpenGLState();
	initFBOs();
	initModels();
	initShaders();
	initUniforms();	

	std::vector<const GLchar*> faces;
	faces.push_back("textures/skybox/right.tga");
	faces.push_back("textures/skybox/left.tga");
	faces.push_back("textures/skybox/top.tga");
	faces.push_back("textures/skybox/bottom.tga");
	faces.push_back("textures/skybox/back.tga");
	faces.push_back("textures/skybox/front.tga");

	mySkyBoxDay.Load(faces);

	std::vector<const GLchar*> faces1;
	faces1.push_back("textures/skyboxN/right.tga");
	faces1.push_back("textures/skyboxN/left.tga");
	faces1.push_back("textures/skyboxN/top.tga");
	faces1.push_back("textures/skyboxN/bottom.tga");
	faces1.push_back("textures/skyboxN/back.tga");
	faces1.push_back("textures/skyboxN/front.tga");

	mySkyBoxNight.Load(faces1);

	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();

	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE,
		glm::value_ptr(view));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE,
		glm::value_ptr(projection));

	glCheckError();

	while (!glfwWindowShouldClose(glWindow)) {
		renderScene();

		glfwPollEvents();
		glfwSwapBuffers(glWindow);

		if (cameraAnimation)
		{
			showCameraAnimation();
		}	
	}

	//close GL context and any other GLFW resources
	glfwTerminate();

	return 0;
}
