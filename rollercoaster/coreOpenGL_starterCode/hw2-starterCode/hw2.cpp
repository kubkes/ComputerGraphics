/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster
  C++ starter code

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "basicPipelineProgram.h"
#include "texturePipelineProgram.h"
#include "linePipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"
#include "math.h"
#include <iostream>
#include <cstring>
#include <vector>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

// ----------------- HW2 Starter Code -------------------
// represents one control point along the spline 
struct Point 
{
  double x;
  double y;
  double z;
};

// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline 
{
  int numControlPoints;
  Point * points;
};

// the spline array 
Spline * splines;
// total number of splines 
int numSplines;

// points on the spline calculated from Catmull-Rom spline equation
// p(u) = [u^3 u^2 u 1] M C
vector <glm::vec3> splineVertices; 

vector <glm::vec3> splineTangents; // tangents of the spline positions
vector <glm::vec3> splineNormals; // normals of the spline positions
vector <glm::vec3> splineBinormals; // binormals of the spline positions

vector <float> dpduVector; // vector to store magnitude of the dp/du values

float maxHeight = 0.0f; // maximum height of the track
float gravity = 9.8f; // acceleration due to gravity

int mousePos[2]; // x,y screen coordinates of the current mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

typedef enum { LINES, TRIANGLES } RENDER_MODE;
RENDER_MODE renderMode = TRIANGLES;

// Transformations of the terrain.
float terrainRotate[3] = { 0.0f, 0.0f, 0.0f }; 
// terrainRotate[0] gives the rotation around x-axis (in degrees)
// terrainRotate[1] gives the rotation around y-axis (in degrees)
// terrainRotate[2] gives the rotation around z-axis (in degrees)
float terrainTranslate[3] = { 0.0f, 0.0f, 0.0f };
float terrainScale[3] = { 1.0f, 1.0f, 1.0f };

// Width and height of the OpenGL window, in pixels.
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework II";

// Texture Handles for cube skybox texture
GLuint skyCubeTexHandle_nx; // negative x
GLuint skyCubeTexHandle_ny; // negative y
GLuint skyCubeTexHandle_nz; // negative z
GLuint skyCubeTexHandle_px; // positive x
GLuint skyCubeTexHandle_py; // positive y
GLuint skyCubeTexHandle_pz; // positive z

// VBO and VAO handles for the texture
GLuint skyCubeTexVBO_nx;
GLuint skyCubeTexVBO_ny;
GLuint skyCubeTexVBO_nz;
GLuint skyCubeTexVBO_px;
GLuint skyCubeTexVBO_py;
GLuint skyCubeTexVBO_pz;

GLuint skyCubeTexVAO_nx;
GLuint skyCubeTexVAO_ny;
GLuint skyCubeTexVAO_nz;
GLuint skyCubeTexVAO_px;
GLuint skyCubeTexVAO_py;
GLuint skyCubeTexVAO_pz;

// VBO and VAO handles for the rail
GLuint leftTubeVBO;
GLuint rightTubeVBO;
GLuint leftBottomVBO;
GLuint rightBottomVBO;
GLuint sleeperVBO;

GLuint leftTubeVAO;
GLuint rightTubeVAO;
GLuint leftBottomVAO;
GLuint rightBottomVAO;
GLuint sleeperVAO;

GLuint lineVBO;
GLuint lineVAO;

int numVertices_line;
int numVertices_leftRail;
int numVertices_rightRail;
int numVertices_sleeper;

int numVertices_skyCubeTex;

int colorChangeEnable; // enable and disable coloring
bool colorFreeze; // freeze the color change if 'f' is pressed, contionue changing color if 'c' is pressed

int autoRideEnable = 0; // enable and disable auto ride
bool autoRide = false; // control for automatic camera movement based on the tracks, start if 'r' is pressed

int camLocIndex = 0;  // camera location index to be used in auto ride

typedef enum { CONSTANT, VARYING } SPEED_STATE; // camere speed mode
SPEED_STATE camSpeedMode = VARYING;

// Initialize eye, center and up vectors for LookAt function
glm::vec3 eye (1.0f, 1.0f, 1.0f);
glm::vec3 center (0.0f);
glm::vec3 up (0.0f, 0.0f, 1.0f);

bool autoSave; // control for starting automatic screenshot save, run if 's' is pressed
float curTime; // current time
float oldTime; // old time
float deltaTime1; // delta time to keep track of camera location
float deltaTime2; // delta time to save automatic screenshots
int frameRatePerSecond = 30; // fps
int numFrames = 1000; // max number of frames to be saved automatically
int frameCounter; 

string tmpCounter;
char ssName[3];
const char *tmpchar = NULL;

// CSCI 420 helper classes.
OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;

TexturePipelineProgram * texturePipelineProgram;

LinePipelineProgram * linePipelineProgram;

// Write a screenshot to the specified filename.
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

void displayFunc()
{
  // This function performs the actual rendering.

  // First, clear the screen.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set up the camera position, focus point, and the up vector.
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();

  if(autoRide && autoRideEnable) // auto ride mode 'r'
  {
    eye = splineVertices[camLocIndex] + 2.0f * splineNormals[camLocIndex];
    center = splineVertices[camLocIndex] + 0.5f * splineNormals[camLocIndex] + 2.0f * splineTangents[camLocIndex];
    up = splineNormals[camLocIndex];

    // Debug
    // cout << "camLoc" << camLocIndex << endl;

    matrix.LookAt(eye[0],     eye[1],     eye[2],
                  center[0],  center[1],  center[2],
                  up[0],      up[1],      up[2]);

    // reset terrain translate, rotate, and scale when ride is started again
    terrainTranslate[0] = 0.0f;
    terrainTranslate[1] = 0.0f;
    terrainTranslate[2] = 0.0f;
    terrainRotate[0] = 0.0f;
    terrainRotate[1] = 0.0f;
    terrainRotate[2] = 0.0f;
    terrainScale[0] = 1.0f;
    terrainScale[1] = 1.0f;
    terrainScale[2] = 1.0f;
  }
  else if(autoRide && autoRideEnable == 0) // auto ride mode is paused with press 't'
  {
    eye = splineVertices[camLocIndex] + 2.0f * splineNormals[camLocIndex];
    center = splineVertices[camLocIndex] + 0.5f * splineNormals[camLocIndex] + 2.0f * splineTangents[camLocIndex];
    up = splineNormals[camLocIndex];

    // Debug
    // cout << "camLoc" << camLocIndex << endl;

    matrix.LookAt(eye[0],     eye[1],     eye[2],
                  center[0],  center[1],  center[2],
                  up[0],      up[1],      up[2]);

    // Translations, rotations and scales.
    matrix.Translate(terrainTranslate[0],terrainTranslate[1],terrainTranslate[2]);
    matrix.Rotate(terrainRotate[0],1,0,0);
    matrix.Rotate(terrainRotate[1],0,1,0);
    matrix.Rotate(terrainRotate[2],0,0,1);
    matrix.Scale(terrainScale[0],terrainScale[1],terrainScale[2]);
  }
  else // no auto ride mode
  {
    eye = splineVertices[0] + 2.0f * splineNormals[0];
    center = splineVertices[0] + 0.5f * splineNormals[0] + 2.0f * splineTangents[0];
    up = splineNormals[0];

    matrix.LookAt(eye[0],     eye[1],     eye[2],
                  center[0],  center[1],  center[2],
                  up[0],      up[1],      up[2]);

    // matrix.LookAt(10.0, 10.0, 10.0, 
    //               20.0, 20.0, 10.0, 
    //               0.0, 0.0, 1.0);

    // Translations, rotations and scales.
    matrix.Translate(terrainTranslate[0],terrainTranslate[1],terrainTranslate[2]);
    matrix.Rotate(terrainRotate[0],1,0,0);
    matrix.Rotate(terrainRotate[1],0,1,0);
    matrix.Rotate(terrainRotate[2],0,0,1);
    matrix.Scale(terrainScale[0],terrainScale[1],terrainScale[2]);
  }

  // Read the current modelview, projection, and normal matrices.
  float modelViewMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(modelViewMatrix);

  float projectionMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(projectionMatrix);

  float normalMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetNormalMatrix(normalMatrix);

  // Bind the pipeline program.
  pipelineProgram->Bind(); 

  // Upload the modelview, projection, and normal matrices to the GPU.
  pipelineProgram->SetModelViewMatrix(modelViewMatrix);
  pipelineProgram->SetProjectionMatrix(projectionMatrix);
  pipelineProgram->SetNormalMatrix(normalMatrix);

  // Bind the pipeline program.
  linePipelineProgram->Bind(); 

  // Upload the modelview and projection matrices to the GPU.
  linePipelineProgram->SetModelViewMatrix(modelViewMatrix);
  linePipelineProgram->SetProjectionMatrix(projectionMatrix);

  switch (renderMode)
  {
    case LINES:
      linePipelineProgram->Bind(); 
      // Execute the rendering.
      glBindVertexArray(lineVAO); // Bind the VAO that we want to render.
      glDrawArrays(GL_LINES, 0, numVertices_line); // Render the VAO, by rendering "numVertices", starting from vertex 0.

    break;

    case TRIANGLES:
      pipelineProgram->Bind(); 
      // Execute the rendering.
      glBindVertexArray(leftTubeVAO); // Bind the VAO that we want to render.
      glDrawArrays(GL_TRIANGLES, 0, numVertices_leftRail); // Render the VAO, by rendering "numVertices", starting from vertex 0.

      glBindVertexArray(rightTubeVAO); // Bind the VAO that we want to render.
      glDrawArrays(GL_TRIANGLES, 0, numVertices_rightRail); // Render the VAO, by rendering "numVertices", starting from vertex 0.

      glBindVertexArray(leftBottomVAO); // Bind the VAO that we want to render.
      glDrawArrays(GL_TRIANGLES, 0, numVertices_leftRail); // Render the VAO, by rendering "numVertices", starting from vertex 0.

      glBindVertexArray(rightBottomVAO); // Bind the VAO that we want to render.
      glDrawArrays(GL_TRIANGLES, 0, numVertices_rightRail); // Render the VAO, by rendering "numVertices", starting from vertex 0.

      glBindVertexArray(sleeperVAO); // Bind the VAO that we want to render.
      glDrawArrays(GL_TRIANGLES, 0, numVertices_sleeper); // Render the VAO, by rendering "numVertices", starting from vertex 0.

    break;
  }


  // Phong Shader
  pipelineProgram->Bind(); 
  // get a handle to the viewLightDirection shader variable
  GLint h_viewLightDirection = glGetUniformLocation(pipelineProgram->GetProgramHandle(),"viewLightDirection"); 

  float lightDirection[3] = {0.0f, 0.0f, 1.0f}; // the “Sun” at noon
  float viewLightDirection[3]; // light direction in the view space
  // the following line is pseudo-code:
  // viewLightDirection = (view * float4(lightDirection, 0.0)).xyz;
  for(int i = 0; i < 3; i++)
  {
    viewLightDirection[i] = modelViewMatrix[i]*lightDirection[0] + modelViewMatrix[i+4]*lightDirection[1] + modelViewMatrix[i+8]*lightDirection[2];
  }

  // upload viewLightDirection to the GPU
  glUniform3fv(h_viewLightDirection, 1, viewLightDirection);

  // Phong shader parameters
  float La[4] = {0.20f, 0.083f, 0.024f, 1.0f}; // ambient color - very dark brownish gray
  float Ld[4] = {0.41f, 0.41f, 0.41f, 1.0f}; // diffuse color - dark gray
  float Ls[4] = {0.82f, 0.82f, 0.82f, 1.0f}; // specular color - light gray

  float ka[4] = {0.83f, 0.83f, 0.83f, 1.0f}; // ambient reflection
  float kd[4] = {0.71f, 0.71f, 0.71f, 1.0f}; // diffuse reflection
  float ks[4] = {0.6f, 0.6f, 0.6f, 1.0f}; // specular reflection
  
  float alpha = 12.0f; // shininess

  // Purple - blue colored rails
  // float La[4] = {0.25f, 0.11f, 0.38f, 1.0f}; // ambient color - purple
  // float Ld[4] = {0.09f, 0.71f, 0.70f, 1.0f}; // diffuse color - mint green
  // float Ls[4] = {0.31f, 0.64f, 0.84f, 1.0f}; // specular color - blue
  // float ka[4] = {0.88f, 0.88f, 0.88f, 1.0f}; // ambient reflection
  // float kd[4] = {0.95f, 0.95f, 0.95f, 1.0f}; // diffuse reflection
  // float ks[4] = {0.85f, 0.85f, 0.85f, 1.0f}; // specular reflection
  // float alpha = 5.0f; // shininess

  // Upload Phong shader parameters to the GPU
  glUniform4fv(glGetUniformLocation(pipelineProgram->GetProgramHandle(), "La"), 1, La);
  glUniform4fv(glGetUniformLocation(pipelineProgram->GetProgramHandle(), "Ld"), 1, Ld);
  glUniform4fv(glGetUniformLocation(pipelineProgram->GetProgramHandle(), "Ls"), 1, Ls);

  glUniform4fv(glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ka"), 1, ka);
  glUniform4fv(glGetUniformLocation(pipelineProgram->GetProgramHandle(), "kd"), 1, kd);
  glUniform4fv(glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ks"), 1, ks);

  glUniform1f(glGetUniformLocation(pipelineProgram->GetProgramHandle(), "alpha"), alpha);

  // ---------- Texture -----------
  // Bind the texture pipeline program.
  texturePipelineProgram->Bind();

  // Upload the modelview and projection matrices to the GPU.
  texturePipelineProgram->SetModelViewMatrix(modelViewMatrix);
  texturePipelineProgram->SetProjectionMatrix(projectionMatrix);

  // Execute the rendering for the cube sky box texture
  glBindTexture(GL_TEXTURE_2D, skyCubeTexHandle_nx);
  glBindVertexArray(skyCubeTexVAO_nx);
  glDrawArrays(GL_TRIANGLES, 0, numVertices_skyCubeTex);

  glBindTexture(GL_TEXTURE_2D, skyCubeTexHandle_ny);
  glBindVertexArray(skyCubeTexVAO_ny);
  glDrawArrays(GL_TRIANGLES, 0, numVertices_skyCubeTex);

  glBindTexture(GL_TEXTURE_2D, skyCubeTexHandle_nz);
  glBindVertexArray(skyCubeTexVAO_nz);
  glDrawArrays(GL_TRIANGLES, 0, numVertices_skyCubeTex);

  glBindTexture(GL_TEXTURE_2D, skyCubeTexHandle_px);
  glBindVertexArray(skyCubeTexVAO_px);
  glDrawArrays(GL_TRIANGLES, 0, numVertices_skyCubeTex);

  glBindTexture(GL_TEXTURE_2D, skyCubeTexHandle_py);
  glBindVertexArray(skyCubeTexVAO_py);
  glDrawArrays(GL_TRIANGLES, 0, numVertices_skyCubeTex);

  glBindTexture(GL_TEXTURE_2D, skyCubeTexHandle_pz);
  glBindVertexArray(skyCubeTexVAO_pz);
  glDrawArrays(GL_TRIANGLES, 0, numVertices_skyCubeTex);

  // Swap the double-buffers.
  glutSwapBuffers();
}

void idleFunc()
{

  // get the current time elpased
  curTime = glutGet(GLUT_ELAPSED_TIME); // milliseconds
  deltaTime1 += (curTime - oldTime)/1000; // delta time in seconds
  deltaTime2 += (curTime - oldTime)/1000; // delta time in seconds
  oldTime = curTime;

  int u_update = 0;

  float timeStep = 5;

  // if autoride is pressed, change the camera position with the speed of frame rate 
  // unless the user stops the movement with the key 't'
  if( autoRideEnable )
  {
    while(deltaTime1 * frameRatePerSecond >= 1)
    {
      deltaTime1 = 0;

      switch (camSpeedMode)
      {
        case VARYING:
          u_update = (int) glm::round(timeStep * glm::sqrt(2*gravity*(maxHeight-splineVertices[camLocIndex][2])) / dpduVector[camLocIndex]);
          camLocIndex = (camLocIndex + u_update) % splineVertices.size(); 
        break;
      
        case CONSTANT:
          u_update = 15;
          camLocIndex = (camLocIndex + u_update) % splineVertices.size();
        break;
      } 
    }
  }

  // if autosave is pressed save screenshot until we reach numFrames
  if( autoSave )
  {
    if(frameCounter == numFrames)
    {
      autoSave = false;
      frameCounter = 0;
    }
    while(deltaTime2 * frameRatePerSecond >= 1)
    {
      deltaTime2 = 0;
      tmpCounter = to_string(frameCounter);
      tmpchar = tmpCounter.c_str();
      strcpy(ssName,tmpchar);
      strcat(ssName,".jpg");
      saveScreenshot(ssName);
      frameCounter++;
    }
  }

  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  // When the window has been resized, we need to re-set our projection matrix.
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  // You need to be careful about setting the zNear and zFar. 
  // Anything closer than zNear, or further than zFar, will be culled.
  const float zNear = 0.1f;
  const float zFar = 10000.0f;
  const float humanFieldOfView = 60.0f;
  matrix.Perspective(humanFieldOfView, 1.0f * w / h, zNear, zFar);
}

void mouseMotionDragFunc(int x, int y)
{
  // Mouse has moved, and one of the mouse buttons is pressed (dragging).

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the terrain
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        terrainTranslate[0] += mousePosDelta[0] * 0.1f;
        terrainTranslate[1] -= mousePosDelta[1] * 0.1f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        terrainTranslate[2] += mousePosDelta[1] * 0.1f;
      }
      break;

    // rotate the terrain
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        terrainRotate[0] += mousePosDelta[1];
        terrainRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        terrainRotate[2] += mousePosDelta[1];
      }

      break;

    // scale the terrain
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        terrainScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        terrainScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        terrainScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;

}

void mouseMotionFunc(int x, int y)
{
  linePipelineProgram->Bind();

  const GLuint locationOfMouseX = glGetUniformLocation(linePipelineProgram->GetProgramHandle(), "mouseX"); // Obtain a handle to the shader variable "colorScale".
  const GLuint locationOfMouseY = glGetUniformLocation(linePipelineProgram->GetProgramHandle(), "mouseY"); // Obtain a handle to the shader variable "colorScale".
 
  // Mouse has moved.
  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;

  if ( colorChangeEnable && !colorFreeze )
  {
    glUniform1f(locationOfMouseX, float(x)/windowWidth);
    glUniform1f(locationOfMouseY, float(y)/windowHeight);
  }
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // A mouse button has has been pressed or depressed.

  // Keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables.
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // Keep track of whether CTRL and SHIFT keys are pressed.
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // If CTRL and SHIFT are not pressed, we are in rotate mode.
    default:
      controlState = ROTATE;
    break;
  }

  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  linePipelineProgram->Bind();

  const GLuint locationOfShaderMode = glGetUniformLocation(linePipelineProgram->GetProgramHandle(), "mode"); // Obtain a handle to the shader variable "mode".
  const GLuint locationOfColorEnable = glGetUniformLocation(linePipelineProgram->GetProgramHandle(), "colorChangeEnable"); // Obtain a handle to the shader variable "colorScale".
  
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // Take a screenshot.
      saveScreenshot("screenshot.jpg");
    break;

    case 's':
      autoSave = true; // start automatic screenshot saving
    break;

    case 'r':
      autoRide = true; // start automatic riding & camera movement
      autoRideEnable = 1; // continue auto ride
    break;

    case 't':
      autoRideEnable = 0; // stop auto ride
    break;

    // Translate when w is pressed (CTRL and ALT did not work with Mac)
    case 'w':
      controlState = TRANSLATE;
    break;

    // Render Mode is Lines when key '2' is pressed
    case '2':
      linePipelineProgram->Bind(); 
      renderMode = LINES;
      glUniform1i(locationOfShaderMode, 0);
      colorChangeEnable = 0; // reset color change
      glUniform1i(locationOfColorEnable, colorChangeEnable);
    break;

    // Render Mode is Triangles when key '3' is pressed
    case '3':
      pipelineProgram->Bind(); 
      renderMode = TRIANGLES;
    break;

    // Color change with the mouse enabled for rendering the line
    case '5':
      linePipelineProgram->Bind(); 
      colorChangeEnable = 1;
      glUniform1i(locationOfColorEnable, colorChangeEnable);
    break;

    // Color change with the mouse is freezed
    case 'f':
      linePipelineProgram->Bind(); 
      colorFreeze = true;
    break;

    // Color change with the mouse is continued
    case 'c':
      linePipelineProgram->Bind(); 
      colorFreeze = false;
    break;

    // camera's speed is constant in u
    case 'u':
      camSpeedMode = CONSTANT;
      cout << "Camera speed mode is changed: constant in u" << endl;
    break;

    // camera's speed is varying in u based on delta_t * sqrt(2*g*(h_max-h))/ mag(dp/du)
    case 'y':
      camSpeedMode = VARYING;
      cout << "Camera speed mode is changed: varying in u" << endl;
    break;    

  }
}

int loadSplines(char * argv) 
{
  char * cName = (char *) malloc(128 * sizeof(char));
  FILE * fileList;
  FILE * fileSpline;
  int iType, i = 0, j, iLength;

  // load the track file 
  fileList = fopen(argv, "r");
  if (fileList == NULL) 
  {
    printf ("can't open file\n");
    exit(1);
  }
  
  // stores the number of splines in a global variable 
  fscanf(fileList, "%d", &numSplines);

  splines = (Spline*) malloc(numSplines * sizeof(Spline));

  // reads through the spline files 
  for (j = 0; j < numSplines; j++) 
  {
    i = 0;
    fscanf(fileList, "%s", cName);
    fileSpline = fopen(cName, "r");

    if (fileSpline == NULL) 
    {
      printf ("can't open file\n");
      exit(1);
    }

    // gets length for spline file
    fscanf(fileSpline, "%d %d", &iLength, &iType);

    // allocate memory for all the points
    splines[j].points = (Point *)malloc(iLength * sizeof(Point));
    splines[j].numControlPoints = iLength;

    // saves the data to the struct
    while (fscanf(fileSpline, "%lf %lf %lf", 
	   &splines[j].points[i].x, 
	   &splines[j].points[i].y, 
	   &splines[j].points[i].z) != EOF) 
    {
      i++;
    }
  }

  free(cName);

  return 0;
}

int initTexture(const char * imageFilename, GLuint textureHandle)
{
  // read the texture image
  ImageIO img;
  ImageIO::fileFormatType imgFormat;
  ImageIO::errorType err = img.load(imageFilename, &imgFormat);

  if (err != ImageIO::OK) 
  {
    printf("Loading texture from %s failed.\n", imageFilename);
    return -1;
  }

  // check that the number of bytes is a multiple of 4
  if (img.getWidth() * img.getBytesPerPixel() % 4) 
  {
    printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
    return -1;
  }

  // allocate space for an array of pixels
  int width = img.getWidth();
  int height = img.getHeight();
  unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

  // fill the pixelsRGBA array with the image pixels
  memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
  for (int h = 0; h < height; h++)
    for (int w = 0; w < width; w++) 
    {
      // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
      pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
      pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
      pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
      pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

      // set the RGBA channels, based on the loaded image
      int numChannels = img.getBytesPerPixel();
      for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
        pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
    }

  // bind the texture
  glBindTexture(GL_TEXTURE_2D, textureHandle);

  // initialize the texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

  // generate the mipmaps for this texture
  glGenerateMipmap(GL_TEXTURE_2D);

  // set the texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // query support for anisotropic texture filtering
  GLfloat fLargest;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
  printf("Max available anisotropic samples: %f\n", fLargest);
  // set anisotropic texture filtering
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

  // query for any errors
  GLenum errCode = glGetError();
  if (errCode != 0) 
  {
    printf("Texture initialization error. Error code: %d.\n", errCode);
    return -1;
  }
  
  // de-allocate the pixel array -- it is no longer needed
  delete [] pixelsRGBA;

  return 0;
}

// normalize vector function to handle nan values
glm::vec3 normalizeVector(glm::vec3 v)
{
  // if the vector is all zeros, return the zero vector, not normalize to avoid nan values
  if(glm::dot(v,v) == 0.0f)
  {
    return v;
  }
  else
  {
    return glm::normalize(v);
  }
}

// Calculate and store spline vertices, tangents, normals, and binormals
void calculateSplineProperties()
{
  int numSteps = 1000; // number of steps between u = 0 and u = 1
  float u_step = 1/(float)numSteps; // u increment

  glm::vec3 initPos(0.0f); // initial position of the spline

  glm::vec4 u_vec(1.0f); // vector to store u^3, u^2, u, 1 

  glm::vec4 ud_vec(0.0f); // vector to store derivative of u_vec; 3u^2, 2u, 1, 0 

  glm::vec3 pos_temp(0.0f); // temporary storage for positions 
  glm::vec3 tan_temp(0.0f); // temporary storage for tangents 
  glm::vec3 nor_temp(0.0f); // temporary storage for normals 
  glm::vec3 bin_temp(0.0f); // temporary storage for binormals 

  float mag_dpdu = 0.0f; // magnitude of the dp/du vector

  glm::vec3 v(0.0f); // arbitrary vector v to start the Sloan's method
  v[1] = 1.0f; 

  float s = 0.5f; // s = 1/2 for Catmull-Rom Spline Matrix
  glm::mat3x4 C(0.0f); // Control matrix (4x3 matrix)
  glm::mat4x4 M(-s,  2*s,   -s,   0.0f,
                2-s, s-3,   0.0f, 1.0f,
                s-2, 3-2*s, s,    0.0f,
                s,   -s,    0.0f, 0.0f); // Spline basis matrix (4x4 matrix)
  glm::mat3x4 MC(0.0f); // Matrix for M * C multiplication

  // loop over each spline
  for(int i = 0; i < numSplines; i++)
  {
    // debug
    // cout << "Spline " << i << endl;

    // loop over each control points in a spline
    for(int j = 1; j < splines[i].numControlPoints-1; j++)
    {
      // debug
      // cout << "x: " << splines[i].points[j].x << ", y: " << splines[i].points[j].y << ", z: " << splines[i].points[j].z << endl;

      // Assigning values to C matrix based on 4 control points
      for(int k = 0; k < 4; k++)
      {
        C[0][k] = (float)splines[i].points[j-(1-k)].x;
        C[1][k] = (float)splines[i].points[j-(1-k)].y;
        C[2][k] = (float)splines[i].points[j-(1-k)].z;
      }

      MC = M * C;

      // loop over all u values
      for(float u = 0.0f; u < 1.0f; u += u_step)
      {
        // Calculate positions
        u_vec[0] = pow(u,3);
        u_vec[1] = pow(u,2);
        u_vec[2] = u;

        pos_temp = u_vec * MC + initPos;

        splineVertices.push_back(pos_temp);

        // update maximum height of the track
        if(pos_temp[2] > maxHeight)
        {
          maxHeight = pos_temp[2];
        }

        // Calculate tangents
        ud_vec[0] = 3*pow(u,2);
        ud_vec[1] = 2*u;
        ud_vec[2] = 1.0f;

        tan_temp = normalizeVector(ud_vec * MC);
        splineTangents.push_back(tan_temp);

        // save magnitude of dp/du vector to be used in camera speed
        mag_dpdu = glm::length(ud_vec * MC);
        dpduVector.push_back(mag_dpdu);

        if( u == 0.0f && j == 1 )
        {
          // Calculate normals
          nor_temp = normalizeVector(glm::cross(tan_temp,v));
          
          splineNormals.push_back(nor_temp);

          // Calculate binormals
          bin_temp = normalizeVector(glm::cross(tan_temp,nor_temp));

          splineBinormals.push_back(bin_temp);
        }
        else
        {  
          // Calculate normals
          nor_temp = normalizeVector(glm::cross(bin_temp,tan_temp));
          
          splineNormals.push_back(nor_temp);

          // Calculate binormals
          bin_temp = normalizeVector(glm::cross(tan_temp,nor_temp));

          splineBinormals.push_back(bin_temp);
        }
      }
    }

    initPos = pos_temp; // update the initial position of the spline
  }

 // add a small value to the max height to not stop updating the u value when we come to the max height location
  maxHeight += 1.0f;

  // Debug
  // cout << "Vertices spline:" << splineVertices.size() << endl;
}

// calculate the normal of the triangle whose vertices are v1, v2, and v3 counter-clockwise
glm::vec3 calculateTriangleNormal(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3)
{
  glm::vec3 u = v2-v1;
  glm::vec3 w = v3-v1;
  glm::vec3 n(0.0f);

  // cross product of u and w
  n[0] = u[1]*w[2] - u[2]*w[1];
  n[1] = u[2]*w[0] - u[0]*w[2];
  n[2] = u[0]*w[1] - u[1]*w[0];

  return normalizeVector(n);
}


// construct the rails and upload to the GPU
void prepareRailRoad()
{

  // T cross-section's top part
  vector <glm::vec3> leftTubePos; // Vertex positions for left rail tube
  vector <glm::vec3> rightTubePos; // Vertex positions for right rail tube
  vector <glm::vec3> leftTubeNormals; // Normals of the triangles for left rail tube
  vector <glm::vec3> rightTubeNormals; // Normals of the triangles for left rail tube

  // T cross-section's bottom part
  vector <glm::vec3> leftBottomPos; // Vertex positions for left rail bottom
  vector <glm::vec3> rightBottomPos; // Vertex positions for right rail bottom
  vector <glm::vec3> leftBottomNormals; // Normals of the triangles for left rail bottom
  vector <glm::vec3> rightBottomNormals; // Normals of the triangles for left rail bottom

  // Sleeper
  vector <glm::vec3> sleeperPos; // Vertex positions for sleeper
  vector <glm::vec3> sleeperNormals; // Normals of the triangles for sleeper
  
  // Create temporary v0,v1,...,v7 vertex positions for rectangular cross section for left tube
  glm::vec3 left_v0(0.0f);
  glm::vec3 left_v1(0.0f);
  glm::vec3 left_v2(0.0f);
  glm::vec3 left_v3(0.0f);
  glm::vec3 left_v4(0.0f);
  glm::vec3 left_v5(0.0f);
  glm::vec3 left_v6(0.0f);
  glm::vec3 left_v7(0.0f);
  
  // Create temporary v0,v1,...,v7 vertex positions for rectangular cross section for right tube
  glm::vec3 right_v0(0.0f);
  glm::vec3 right_v1(0.0f);
  glm::vec3 right_v2(0.0f);
  glm::vec3 right_v3(0.0f);
  glm::vec3 right_v4(0.0f);
  glm::vec3 right_v5(0.0f);
  glm::vec3 right_v6(0.0f);
  glm::vec3 right_v7(0.0f);

  // Create temporary v0,v1,...,v7 vertex positions for sleepers
  glm::vec3 temp_v0(0.0f);
  glm::vec3 temp_v1(0.0f);
  glm::vec3 temp_v2(0.0f);
  glm::vec3 temp_v3(0.0f);
  glm::vec3 temp_v4(0.0f);
  glm::vec3 temp_v5(0.0f);
  glm::vec3 temp_v6(0.0f);
  glm::vec3 temp_v7(0.0f);

  // Temporary p0 and p1 positions for v0,...,v7 calculations
  glm::vec3 temp_p0(0.0f);
  glm::vec3 temp_p1(0.0f);

  // Temporary n0 and n1 normals for v0,...,v7 calculations
  glm::vec3 temp_n0(0.0f);
  glm::vec3 temp_n1(0.0f);

  // Temporary b0 and b1 binormals for v0,...,v7 calculations
  glm::vec3 temp_b0(0.0f);
  glm::vec3 temp_b1(0.0f);

  // Tempopary vector to store normal of a triangle
  glm::vec3 temp_nor(0.0f);

  // T cross-section's top part 
  // To make the cross section rectangular, weighting of N and B will be different 
  // instead of multiplying both with alpha
  float alphaN = 0.04f;
  float alphaB = 0.05f;

  // Half of the distance between two rail tubes from p0 to other p0, or vi to other vi
  float tubeHalfDist = 0.5f;

  for (int i = 0; i < splineVertices.size()-1; i++)
  {
    temp_p0 = splineVertices[i];
    temp_p1 = splineVertices[i+1];

    temp_n0 = splineNormals[i];
    temp_n1 = splineNormals[i+1];

    temp_b0 = splineBinormals[i];
    temp_b1 = splineBinormals[i+1];

    left_v0 = temp_p0 - 1.0f * alphaN * temp_n0 + 1.0f * alphaB * temp_b0 - tubeHalfDist * temp_b0;
    left_v1 = temp_p0 + 1.0f * alphaN * temp_n0 + 1.0f * alphaB * temp_b0 - tubeHalfDist * temp_b0;
    left_v2 = temp_p0 + 1.0f * alphaN * temp_n0 - 1.0f * alphaB * temp_b0 - tubeHalfDist * temp_b0;
    left_v3 = temp_p0 - 1.0f * alphaN * temp_n0 - 1.0f * alphaB * temp_b0 - tubeHalfDist * temp_b0;

    left_v4 = temp_p1 - 1.0f * alphaN * temp_n1 + 1.0f * alphaB * temp_b1 - tubeHalfDist * temp_b1;
    left_v5 = temp_p1 + 1.0f * alphaN * temp_n1 + 1.0f * alphaB * temp_b1 - tubeHalfDist * temp_b1;
    left_v6 = temp_p1 + 1.0f * alphaN * temp_n1 - 1.0f * alphaB * temp_b1 - tubeHalfDist * temp_b1;
    left_v7 = temp_p1 - 1.0f * alphaN * temp_n1 - 1.0f * alphaB * temp_b1 - tubeHalfDist * temp_b1;

    right_v0 = temp_p0 - 1.0f * alphaN * temp_n0 + 1.0f * alphaB * temp_b0 + tubeHalfDist * temp_b0;
    right_v1 = temp_p0 + 1.0f * alphaN * temp_n0 + 1.0f * alphaB * temp_b0 + tubeHalfDist * temp_b0;
    right_v2 = temp_p0 + 1.0f * alphaN * temp_n0 - 1.0f * alphaB * temp_b0 + tubeHalfDist * temp_b0;
    right_v3 = temp_p0 - 1.0f * alphaN * temp_n0 - 1.0f * alphaB * temp_b0 + tubeHalfDist * temp_b0;

    right_v4 = temp_p1 - 1.0f * alphaN * temp_n1 + 1.0f * alphaB * temp_b1 + tubeHalfDist * temp_b1;
    right_v5 = temp_p1 + 1.0f * alphaN * temp_n1 + 1.0f * alphaB * temp_b1 + tubeHalfDist * temp_b1;
    right_v6 = temp_p1 + 1.0f * alphaN * temp_n1 - 1.0f * alphaB * temp_b1 + tubeHalfDist * temp_b1;
    right_v7 = temp_p1 - 1.0f * alphaN * temp_n1 - 1.0f * alphaB * temp_b1 + tubeHalfDist * temp_b1;

    // Triangle 1 - v0,v4,v1 (Right side)
    leftTubePos.push_back(left_v0);
    leftTubePos.push_back(left_v4);
    leftTubePos.push_back(left_v1);

    rightTubePos.push_back(right_v0);
    rightTubePos.push_back(right_v4);
    rightTubePos.push_back(right_v1);   

    temp_nor = calculateTriangleNormal(left_v0, left_v4, left_v1);
    leftTubeNormals.push_back(temp_nor);
    leftTubeNormals.push_back(temp_nor);
    leftTubeNormals.push_back(temp_nor);

    temp_nor = calculateTriangleNormal(right_v0, right_v4, right_v1);
    rightTubeNormals.push_back(temp_nor);
    rightTubeNormals.push_back(temp_nor);
    rightTubeNormals.push_back(temp_nor);

    // Triangle 2 - v1,v4,v5 (Right side)
    leftTubePos.push_back(left_v1);
    leftTubePos.push_back(left_v4);
    leftTubePos.push_back(left_v5);

    rightTubePos.push_back(right_v1);
    rightTubePos.push_back(right_v4);
    rightTubePos.push_back(right_v5);   

    temp_nor = calculateTriangleNormal(left_v1, left_v4, left_v5);
    leftTubeNormals.push_back(temp_nor);
    leftTubeNormals.push_back(temp_nor);
    leftTubeNormals.push_back(temp_nor);

    temp_nor = calculateTriangleNormal(right_v1, right_v4, right_v5);
    rightTubeNormals.push_back(temp_nor);
    rightTubeNormals.push_back(temp_nor);
    rightTubeNormals.push_back(temp_nor);

    // Triangle 3 - v1,v5,v2 (Top side)
    leftTubePos.push_back(left_v1);
    leftTubePos.push_back(left_v5);
    leftTubePos.push_back(left_v2);

    rightTubePos.push_back(right_v1);
    rightTubePos.push_back(right_v5);
    rightTubePos.push_back(right_v2);   

    temp_nor = calculateTriangleNormal(left_v1, left_v5, left_v2);
    leftTubeNormals.push_back(temp_nor);
    leftTubeNormals.push_back(temp_nor);
    leftTubeNormals.push_back(temp_nor);

    temp_nor = calculateTriangleNormal(right_v1, right_v5, right_v2);
    rightTubeNormals.push_back(temp_nor);
    rightTubeNormals.push_back(temp_nor);
    rightTubeNormals.push_back(temp_nor);

    // Triangle 4 - v2,v5,v6 (Top side)
    leftTubePos.push_back(left_v2);
    leftTubePos.push_back(left_v5);
    leftTubePos.push_back(left_v6);

    rightTubePos.push_back(right_v2);
    rightTubePos.push_back(right_v5);
    rightTubePos.push_back(right_v6);   

    temp_nor = calculateTriangleNormal(left_v2, left_v5, left_v6);
    leftTubeNormals.push_back(temp_nor);
    leftTubeNormals.push_back(temp_nor);
    leftTubeNormals.push_back(temp_nor);

    temp_nor = calculateTriangleNormal(right_v2, right_v5, right_v6);
    rightTubeNormals.push_back(temp_nor);
    rightTubeNormals.push_back(temp_nor);
    rightTubeNormals.push_back(temp_nor);

    // Triangle 5 - v2,v6,v3 (Left side)
    leftTubePos.push_back(left_v2);
    leftTubePos.push_back(left_v6);
    leftTubePos.push_back(left_v3);

    rightTubePos.push_back(right_v2);
    rightTubePos.push_back(right_v6);
    rightTubePos.push_back(right_v3);   

    temp_nor = calculateTriangleNormal(left_v2, left_v6, left_v3);
    leftTubeNormals.push_back(temp_nor);
    leftTubeNormals.push_back(temp_nor);
    leftTubeNormals.push_back(temp_nor);

    temp_nor = calculateTriangleNormal(right_v2, right_v6, right_v3);
    rightTubeNormals.push_back(temp_nor);
    rightTubeNormals.push_back(temp_nor);
    rightTubeNormals.push_back(temp_nor);

    // Triangle 6 - v3,v6,v7 (Left side)
    leftTubePos.push_back(left_v3);
    leftTubePos.push_back(left_v6);
    leftTubePos.push_back(left_v7);

    rightTubePos.push_back(right_v3);
    rightTubePos.push_back(right_v6);
    rightTubePos.push_back(right_v7);   

    temp_nor = calculateTriangleNormal(left_v3, left_v6, left_v7);
    leftTubeNormals.push_back(temp_nor);
    leftTubeNormals.push_back(temp_nor);
    leftTubeNormals.push_back(temp_nor);

    temp_nor = calculateTriangleNormal(right_v3, right_v6, right_v7);
    rightTubeNormals.push_back(temp_nor);
    rightTubeNormals.push_back(temp_nor);
    rightTubeNormals.push_back(temp_nor);

    // Triangle 7 - v3,v7,v0 (Bottom side)
    leftTubePos.push_back(left_v3);
    leftTubePos.push_back(left_v7);
    leftTubePos.push_back(left_v0);

    rightTubePos.push_back(right_v3);
    rightTubePos.push_back(right_v7);
    rightTubePos.push_back(right_v0);   

    temp_nor = calculateTriangleNormal(left_v3, left_v7, left_v0);
    leftTubeNormals.push_back(temp_nor);
    leftTubeNormals.push_back(temp_nor);
    leftTubeNormals.push_back(temp_nor);

    temp_nor = calculateTriangleNormal(right_v3, right_v7, right_v0);
    rightTubeNormals.push_back(temp_nor);
    rightTubeNormals.push_back(temp_nor);
    rightTubeNormals.push_back(temp_nor);

    // Triangle 7 - v0,v7,v4 (Bottom side)
    leftTubePos.push_back(left_v0);
    leftTubePos.push_back(left_v7);
    leftTubePos.push_back(left_v4);

    rightTubePos.push_back(right_v0);
    rightTubePos.push_back(right_v7);
    rightTubePos.push_back(right_v4);   

    temp_nor = calculateTriangleNormal(left_v0, left_v7, left_v4);
    leftTubeNormals.push_back(temp_nor);
    leftTubeNormals.push_back(temp_nor);
    leftTubeNormals.push_back(temp_nor);

    temp_nor = calculateTriangleNormal(right_v0, right_v7, right_v4);
    rightTubeNormals.push_back(temp_nor);
    rightTubeNormals.push_back(temp_nor);
    rightTubeNormals.push_back(temp_nor);

  }

  // T cross-section's bottom part 
  // To make the cross section rectangular, weighting of N and B will be different 
  // instead of multiplying both with beta
  float betaN = 0.02f;
  float betaB = 0.12f;

  for (int i = 0; i < splineVertices.size()-1; i++)
  {
    temp_n0 = splineNormals[i];
    temp_n1 = splineNormals[i+1];

    // center of the bottom T cross-section is little down in the negative direction of the normals
    temp_p0 = splineVertices[i] - (alphaN + betaN) * temp_n0; 
    temp_p1 = splineVertices[i+1] - (alphaN + betaN) * temp_n1;

    temp_b0 = splineBinormals[i];
    temp_b1 = splineBinormals[i+1];

    left_v0 = temp_p0 - 1.0f * betaN * temp_n0 + 1.0f * betaB * temp_b0 - tubeHalfDist * temp_b0;
    left_v1 = temp_p0 + 1.0f * betaN * temp_n0 + 1.0f * betaB * temp_b0 - tubeHalfDist * temp_b0;
    left_v2 = temp_p0 + 1.0f * betaN * temp_n0 - 1.0f * betaB * temp_b0 - tubeHalfDist * temp_b0;
    left_v3 = temp_p0 - 1.0f * betaN * temp_n0 - 1.0f * betaB * temp_b0 - tubeHalfDist * temp_b0;

    left_v4 = temp_p1 - 1.0f * betaN * temp_n1 + 1.0f * betaB * temp_b1 - tubeHalfDist * temp_b1;
    left_v5 = temp_p1 + 1.0f * betaN * temp_n1 + 1.0f * betaB * temp_b1 - tubeHalfDist * temp_b1;
    left_v6 = temp_p1 + 1.0f * betaN * temp_n1 - 1.0f * betaB * temp_b1 - tubeHalfDist * temp_b1;
    left_v7 = temp_p1 - 1.0f * betaN * temp_n1 - 1.0f * betaB * temp_b1 - tubeHalfDist * temp_b1;

    right_v0 = temp_p0 - 1.0f * betaN * temp_n0 + 1.0f * betaB * temp_b0 + tubeHalfDist * temp_b0;
    right_v1 = temp_p0 + 1.0f * betaN * temp_n0 + 1.0f * betaB * temp_b0 + tubeHalfDist * temp_b0;
    right_v2 = temp_p0 + 1.0f * betaN * temp_n0 - 1.0f * betaB * temp_b0 + tubeHalfDist * temp_b0;
    right_v3 = temp_p0 - 1.0f * betaN * temp_n0 - 1.0f * betaB * temp_b0 + tubeHalfDist * temp_b0;

    right_v4 = temp_p1 - 1.0f * betaN * temp_n1 + 1.0f * betaB * temp_b1 + tubeHalfDist * temp_b1;
    right_v5 = temp_p1 + 1.0f * betaN * temp_n1 + 1.0f * betaB * temp_b1 + tubeHalfDist * temp_b1;
    right_v6 = temp_p1 + 1.0f * betaN * temp_n1 - 1.0f * betaB * temp_b1 + tubeHalfDist * temp_b1;
    right_v7 = temp_p1 - 1.0f * betaN * temp_n1 - 1.0f * betaB * temp_b1 + tubeHalfDist * temp_b1;

    // Triangle 1 - v0,v4,v1 (Right side)
    leftBottomPos.push_back(left_v0);
    leftBottomPos.push_back(left_v4);
    leftBottomPos.push_back(left_v1);

    rightBottomPos.push_back(right_v0);
    rightBottomPos.push_back(right_v4);
    rightBottomPos.push_back(right_v1);   

    temp_nor = calculateTriangleNormal(left_v0, left_v4, left_v1);
    leftBottomNormals.push_back(temp_nor);
    leftBottomNormals.push_back(temp_nor);
    leftBottomNormals.push_back(temp_nor);

    temp_nor = calculateTriangleNormal(right_v0, right_v4, right_v1);
    rightBottomNormals.push_back(temp_nor);
    rightBottomNormals.push_back(temp_nor);
    rightBottomNormals.push_back(temp_nor);

    // Triangle 2 - v1,v4,v5 (Right side)
    leftBottomPos.push_back(left_v1);
    leftBottomPos.push_back(left_v4);
    leftBottomPos.push_back(left_v5);

    rightBottomPos.push_back(right_v1);
    rightBottomPos.push_back(right_v4);
    rightBottomPos.push_back(right_v5);   

    temp_nor = calculateTriangleNormal(left_v1, left_v4, left_v5);
    leftBottomNormals.push_back(temp_nor);
    leftBottomNormals.push_back(temp_nor);
    leftBottomNormals.push_back(temp_nor);

    temp_nor = calculateTriangleNormal(right_v1, right_v4, right_v5);
    rightBottomNormals.push_back(temp_nor);
    rightBottomNormals.push_back(temp_nor);
    rightBottomNormals.push_back(temp_nor);

    // Triangle 3 - v1,v5,v2 (Top side)
    leftBottomPos.push_back(left_v1);
    leftBottomPos.push_back(left_v5);
    leftBottomPos.push_back(left_v2);

    rightBottomPos.push_back(right_v1);
    rightBottomPos.push_back(right_v5);
    rightBottomPos.push_back(right_v2);   

    temp_nor = calculateTriangleNormal(left_v1, left_v5, left_v2);
    leftBottomNormals.push_back(temp_nor);
    leftBottomNormals.push_back(temp_nor);
    leftBottomNormals.push_back(temp_nor);

    temp_nor = calculateTriangleNormal(right_v1, right_v5, right_v2);
    rightBottomNormals.push_back(temp_nor);
    rightBottomNormals.push_back(temp_nor);
    rightBottomNormals.push_back(temp_nor);

    // Triangle 4 - v2,v5,v6 (Top side)
    leftBottomPos.push_back(left_v2);
    leftBottomPos.push_back(left_v5);
    leftBottomPos.push_back(left_v6);

    rightBottomPos.push_back(right_v2);
    rightBottomPos.push_back(right_v5);
    rightBottomPos.push_back(right_v6);   

    temp_nor = calculateTriangleNormal(left_v2, left_v5, left_v6);
    leftBottomNormals.push_back(temp_nor);
    leftBottomNormals.push_back(temp_nor);
    leftBottomNormals.push_back(temp_nor);

    temp_nor = calculateTriangleNormal(right_v2, right_v5, right_v6);
    rightBottomNormals.push_back(temp_nor);
    rightBottomNormals.push_back(temp_nor);
    rightBottomNormals.push_back(temp_nor);

    // Triangle 5 - v2,v6,v3 (Left side)
    leftBottomPos.push_back(left_v2);
    leftBottomPos.push_back(left_v6);
    leftBottomPos.push_back(left_v3);

    rightBottomPos.push_back(right_v2);
    rightBottomPos.push_back(right_v6);
    rightBottomPos.push_back(right_v3);   

    temp_nor = calculateTriangleNormal(left_v2, left_v6, left_v3);
    leftBottomNormals.push_back(temp_nor);
    leftBottomNormals.push_back(temp_nor);
    leftBottomNormals.push_back(temp_nor);

    temp_nor = calculateTriangleNormal(right_v2, right_v6, right_v3);
    rightBottomNormals.push_back(temp_nor);
    rightBottomNormals.push_back(temp_nor);
    rightBottomNormals.push_back(temp_nor);

    // Triangle 6 - v3,v6,v7 (Left side)
    leftBottomPos.push_back(left_v3);
    leftBottomPos.push_back(left_v6);
    leftBottomPos.push_back(left_v7);

    rightBottomPos.push_back(right_v3);
    rightBottomPos.push_back(right_v6);
    rightBottomPos.push_back(right_v7);   

    temp_nor = calculateTriangleNormal(left_v3, left_v6, left_v7);
    leftBottomNormals.push_back(temp_nor);
    leftBottomNormals.push_back(temp_nor);
    leftBottomNormals.push_back(temp_nor);

    temp_nor = calculateTriangleNormal(right_v3, right_v6, right_v7);
    rightBottomNormals.push_back(temp_nor);
    rightBottomNormals.push_back(temp_nor);
    rightBottomNormals.push_back(temp_nor);

    // Triangle 7 - v3,v7,v0 (Bottom side)
    leftBottomPos.push_back(left_v3);
    leftBottomPos.push_back(left_v7);
    leftBottomPos.push_back(left_v0);

    rightBottomPos.push_back(right_v3);
    rightBottomPos.push_back(right_v7);
    rightBottomPos.push_back(right_v0);   

    temp_nor = calculateTriangleNormal(left_v3, left_v7, left_v0);
    leftBottomNormals.push_back(temp_nor);
    leftBottomNormals.push_back(temp_nor);
    leftBottomNormals.push_back(temp_nor);

    temp_nor = calculateTriangleNormal(right_v3, right_v7, right_v0);
    rightBottomNormals.push_back(temp_nor);
    rightBottomNormals.push_back(temp_nor);
    rightBottomNormals.push_back(temp_nor);

    // Triangle 7 - v0,v7,v4 (Bottom side)
    leftBottomPos.push_back(left_v0);
    leftBottomPos.push_back(left_v7);
    leftBottomPos.push_back(left_v4);

    rightBottomPos.push_back(right_v0);
    rightBottomPos.push_back(right_v7);
    rightBottomPos.push_back(right_v4);   

    temp_nor = calculateTriangleNormal(left_v0, left_v7, left_v4);
    leftBottomNormals.push_back(temp_nor);
    leftBottomNormals.push_back(temp_nor);
    leftBottomNormals.push_back(temp_nor);

    temp_nor = calculateTriangleNormal(right_v0, right_v7, right_v4);
    rightBottomNormals.push_back(temp_nor);
    rightBottomNormals.push_back(temp_nor);
    rightBottomNormals.push_back(temp_nor);

  }


  // Sleeper
  float thetaN = 0.02f;
  float thetaB = 0.1f + tubeHalfDist + betaB;

  int sleeperSkip = 30;

  // skip #sleeperSkip points to draw the sleepers
  for (int i = 0; i < splineVertices.size()-sleeperSkip; i=i+sleeperSkip*3)
  {
    temp_n0 = splineNormals[i];
    temp_n1 = splineNormals[i+sleeperSkip];

    // center of the bottom T cross-section is little down in the negative direction of the normals
    temp_p0 = splineVertices[i] - (alphaN + 2 * betaN + thetaN) * temp_n0; 
    temp_p1 = splineVertices[i+sleeperSkip] - (alphaN + 2 * betaN + thetaN) * temp_n1;

    temp_b0 = splineBinormals[i];
    temp_b1 = splineBinormals[i+sleeperSkip];

    temp_v0 = temp_p0 - 1.0f * thetaN * temp_n0 + 1.0f * thetaB * temp_b0;
    temp_v1 = temp_p0 + 1.0f * thetaN * temp_n0 + 1.0f * thetaB * temp_b0;
    temp_v2 = temp_p0 + 1.0f * thetaN * temp_n0 - 1.0f * thetaB * temp_b0;
    temp_v3 = temp_p0 - 1.0f * thetaN * temp_n0 - 1.0f * thetaB * temp_b0;

    temp_v4 = temp_p1 - 1.0f * thetaN * temp_n1 + 1.0f * thetaB * temp_b1;
    temp_v5 = temp_p1 + 1.0f * thetaN * temp_n1 + 1.0f * thetaB * temp_b1;
    temp_v6 = temp_p1 + 1.0f * thetaN * temp_n1 - 1.0f * thetaB * temp_b1;
    temp_v7 = temp_p1 - 1.0f * thetaN * temp_n1 - 1.0f * thetaB * temp_b1;

    // Triangle 1 - v0,v4,v1 (Right side)
    sleeperPos.push_back(temp_v0);
    sleeperPos.push_back(temp_v4);
    sleeperPos.push_back(temp_v1);

    temp_nor = calculateTriangleNormal(temp_v0, temp_v4, temp_v1);
    sleeperNormals.push_back(temp_nor);
    sleeperNormals.push_back(temp_nor);
    sleeperNormals.push_back(temp_nor);

    // Triangle 2 - v1,v4,v5 (Right side)
    sleeperPos.push_back(temp_v1);
    sleeperPos.push_back(temp_v4);
    sleeperPos.push_back(temp_v5);

    temp_nor = calculateTriangleNormal(temp_v1, temp_v4, temp_v5);
    sleeperNormals.push_back(temp_nor);
    sleeperNormals.push_back(temp_nor);
    sleeperNormals.push_back(temp_nor);

    // Triangle 3 - v1,v5,v2 (Top side)
    sleeperPos.push_back(temp_v1);
    sleeperPos.push_back(temp_v5);
    sleeperPos.push_back(temp_v2);

    temp_nor = calculateTriangleNormal(temp_v1, temp_v5, temp_v2);
    sleeperNormals.push_back(temp_nor);
    sleeperNormals.push_back(temp_nor);
    sleeperNormals.push_back(temp_nor);

    // Triangle 4 - v2,v5,v6 (Top side)
    sleeperPos.push_back(temp_v2);
    sleeperPos.push_back(temp_v5);
    sleeperPos.push_back(temp_v6); 

    temp_nor = calculateTriangleNormal(temp_v2, temp_v5, temp_v6);
    sleeperNormals.push_back(temp_nor);
    sleeperNormals.push_back(temp_nor);
    sleeperNormals.push_back(temp_nor);

    // Triangle 5 - v2,v6,v3 (Left side)
    sleeperPos.push_back(temp_v2);
    sleeperPos.push_back(temp_v6);
    sleeperPos.push_back(temp_v3); 

    temp_nor = calculateTriangleNormal(temp_v2, temp_v6, temp_v3);
    sleeperNormals.push_back(temp_nor);
    sleeperNormals.push_back(temp_nor);
    sleeperNormals.push_back(temp_nor);

    // Triangle 6 - v3,v6,v7 (Left side)
    sleeperPos.push_back(temp_v3);
    sleeperPos.push_back(temp_v6);
    sleeperPos.push_back(temp_v7); 

    temp_nor = calculateTriangleNormal(temp_v3, temp_v6, temp_v7);
    sleeperNormals.push_back(temp_nor);
    sleeperNormals.push_back(temp_nor);
    sleeperNormals.push_back(temp_nor);

    // Triangle 7 - v3,v7,v0 (Bottom side)
    sleeperPos.push_back(temp_v3);
    sleeperPos.push_back(temp_v7);
    sleeperPos.push_back(temp_v0);

    temp_nor = calculateTriangleNormal(temp_v3, temp_v7, temp_v0);
    sleeperNormals.push_back(temp_nor);
    sleeperNormals.push_back(temp_nor);
    sleeperNormals.push_back(temp_nor);

    // Triangle 7 - v0,v7,v4 (Bottom side)
    sleeperPos.push_back(temp_v0);
    sleeperPos.push_back(temp_v7);
    sleeperPos.push_back(temp_v4);

    temp_nor = calculateTriangleNormal(temp_v0, temp_v7, temp_v4);
    sleeperNormals.push_back(temp_nor);
    sleeperNormals.push_back(temp_nor);
    sleeperNormals.push_back(temp_nor);
  }

  // Prepare VBO and VAO
  numVertices_leftRail = leftTubePos.size();
  numVertices_rightRail = rightTubePos.size();
  numVertices_sleeper = sleeperPos.size();

  int numBytesInPositionsSleeper = sizeof(glm::vec3) * numVertices_sleeper;
  int numBytesInColorsSleeper = sizeof(glm::vec3) * sleeperNormals.size();

  int numBytesInPositions = sizeof(glm::vec3) * numVertices_leftRail;
  int numBytesInColors = sizeof(glm::vec3) * leftTubeNormals.size();

  const int stride = 0; // Stride is 0, i.e., data is tightly packed in the VBO.
  const GLboolean normalized = GL_FALSE; // Normalization is off.

  // Create the VBO for left tube rail
  glGenBuffers(1, &leftTubeVBO);
  glBindBuffer(GL_ARRAY_BUFFER, leftTubeVBO);
  // Allocate an empty VBO of the correct size to hold positions and normals.
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions + numBytesInColors, nullptr, GL_STATIC_DRAW);
  // Write the position and normals data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositions, leftTubePos.data()); // The VBO starts with positions.
  glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositions, numBytesInColors, leftTubeNormals.data()); // The colors are written after the positions.

  // Create the VBO for right tube rail
  glGenBuffers(1, &rightTubeVBO);
  glBindBuffer(GL_ARRAY_BUFFER, rightTubeVBO);
  // Allocate an empty VBO of the correct size to hold positions and normals.
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions + numBytesInColors, nullptr, GL_STATIC_DRAW);
  // Write the position and normals data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositions, rightTubePos.data()); // The VBO starts with positions.
  glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositions, numBytesInColors, rightTubeNormals.data()); // The colors are written after the positions.

  // Create the VBO for left tube rail bottom
  glGenBuffers(1, &leftBottomVBO);
  glBindBuffer(GL_ARRAY_BUFFER, leftBottomVBO);
  // Allocate an empty VBO of the correct size to hold positions and normals.
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions + numBytesInColors, nullptr, GL_STATIC_DRAW);
  // Write the position and normals data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositions, leftBottomPos.data()); // The VBO starts with positions.
  glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositions, numBytesInColors, leftBottomNormals.data()); // The colors are written after the positions.

  // Create the VBO for right tube rail bottom
  glGenBuffers(1, &rightBottomVBO);
  glBindBuffer(GL_ARRAY_BUFFER, rightBottomVBO);
  // Allocate an empty VBO of the correct size to hold positions and normals.
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions + numBytesInColors, nullptr, GL_STATIC_DRAW);
  // Write the position and normals data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositions, rightBottomPos.data()); // The VBO starts with positions.
  glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositions, numBytesInColors, rightBottomNormals.data()); // The colors are written after the positions.

  // Create the VBO for sleeper
  glGenBuffers(1, &sleeperVBO);
  glBindBuffer(GL_ARRAY_BUFFER, sleeperVBO);
  // Allocate an empty VBO of the correct size to hold positions and normals.
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositionsSleeper + numBytesInColorsSleeper, nullptr, GL_STATIC_DRAW);
  // Write the position and normals data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositionsSleeper, sleeperPos.data()); // The VBO starts with positions.
  glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositionsSleeper, numBytesInColorsSleeper, sleeperNormals.data()); // The colors are written after the positions.


  // Create the VAO for left tube rail
  glGenVertexArrays(1, &leftTubeVAO);
  glBindVertexArray(leftTubeVAO);
  glBindBuffer(GL_ARRAY_BUFFER, leftTubeVBO); // The VBO that we bind here will be used in the glVertexAttribPointer calls below. If we forget to bind the VBO here, the program will malfunction.

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint line_locationOfLeftPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(line_locationOfLeftPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfLeftPosition, 3, GL_FLOAT, normalized, stride, (const void *)0); // The shader variable "position" 

  // Set up the relationship between the "color" shader variable and the VAO.
  const GLuint line_locationOfLeftColor = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal"); // Obtain a handle to the shader variable "color".
  glEnableVertexAttribArray(line_locationOfLeftColor); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfLeftColor, 3, GL_FLOAT, normalized, stride, (const void *)(unsigned long)numBytesInPositions); // The shader variable "color" 

  // Create the VAO for right tube rail
  glGenVertexArrays(1, &rightTubeVAO);
  glBindVertexArray(rightTubeVAO);
  glBindBuffer(GL_ARRAY_BUFFER, rightTubeVBO); // The VBO that we bind here will be used in the glVertexAttribPointer calls below. If we forget to bind the VBO here, the program will malfunction.

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint line_locationOfRightPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(line_locationOfRightPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfRightPosition, 3, GL_FLOAT, normalized, stride, (const void *)0); // The shader variable "position" 

  // Set up the relationship between the "color" shader variable and the VAO.
  const GLuint line_locationOfRightColor = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal"); // Obtain a handle to the shader variable "color".
  glEnableVertexAttribArray(line_locationOfRightColor); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfRightColor, 3, GL_FLOAT, normalized, stride, (const void *)(unsigned long)numBytesInPositions); // The shader variable "color" 

  // Create the VAO for left tube rail bottom
  glGenVertexArrays(1, &leftBottomVAO);
  glBindVertexArray(leftBottomVAO);
  glBindBuffer(GL_ARRAY_BUFFER, leftBottomVBO); // The VBO that we bind here will be used in the glVertexAttribPointer calls below. If we forget to bind the VBO here, the program will malfunction.

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint line_locationOfLeftBottomPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(line_locationOfLeftBottomPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfLeftBottomPosition, 3, GL_FLOAT, normalized, stride, (const void *)0); // The shader variable "position" 

  // Set up the relationship between the "color" shader variable and the VAO.
  const GLuint line_locationOfLeftBottomColor = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal"); // Obtain a handle to the shader variable "color".
  glEnableVertexAttribArray(line_locationOfLeftBottomColor); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfLeftBottomColor, 3, GL_FLOAT, normalized, stride, (const void *)(unsigned long)numBytesInPositions); // The shader variable "color" 

  // Create the VAO for right tube rail bottom
  glGenVertexArrays(1, &rightBottomVAO);
  glBindVertexArray(rightBottomVAO);
  glBindBuffer(GL_ARRAY_BUFFER, rightBottomVBO); // The VBO that we bind here will be used in the glVertexAttribPointer calls below. If we forget to bind the VBO here, the program will malfunction.

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint line_locationOfRightBottomPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(line_locationOfRightBottomPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfRightBottomPosition, 3, GL_FLOAT, normalized, stride, (const void *)0); // The shader variable "position" 

  // Set up the relationship between the "color" shader variable and the VAO.
  const GLuint line_locationOfRightBottomColor = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal"); // Obtain a handle to the shader variable "color".
  glEnableVertexAttribArray(line_locationOfRightBottomColor); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfRightBottomColor, 3, GL_FLOAT, normalized, stride, (const void *)(unsigned long)numBytesInPositions); // The shader variable "color" 

  // Create the VAO for sleeper
  glGenVertexArrays(1, &sleeperVAO);
  glBindVertexArray(sleeperVAO);
  glBindBuffer(GL_ARRAY_BUFFER, sleeperVBO); // The VBO that we bind here will be used in the glVertexAttribPointer calls below. If we forget to bind the VBO here, the program will malfunction.

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint line_locationOfSleeperPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(line_locationOfSleeperPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfSleeperPosition, 3, GL_FLOAT, normalized, stride, (const void *)0); // The shader variable "position" 

  // Set up the relationship between the "color" shader variable and the VAO.
  const GLuint line_locationOfSleeperColor = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "normal"); // Obtain a handle to the shader variable "color".
  glEnableVertexAttribArray(line_locationOfSleeperColor); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfSleeperColor, 3, GL_FLOAT, normalized, stride, (const void *)(unsigned long)numBytesInPositionsSleeper); // The shader variable "color" 


}

// Milestone 1, splines displayed with lines
void prepareSplinesLineMode()
{
    // Vertex positions
  float * positions;
  // Vertex colors
  float * colors;

  int numBytesInPositions; 
  int numBytesInColors;

  const int stride = 0; // Stride is 0, i.e., data is tightly packed in the VBO.
  const GLboolean normalized = GL_FALSE; // Normalization is off.

  //-------------------- RENDER MODE: LINES (Key '2') -----------------------

  int numSteps = 1000;
  float u_step = 1/(float)numSteps;

  glm::vec4 u_vec(1.0f); // vector to store u^3, u^2, u, 1 

  glm::vec3 initPos(0.0f); // initial position of the spline

  glm::vec3 pos_temp(0.0f); // temporary storage for positions 

  numVertices_line = 0;

  for (int i = 0; i < numSplines; i++)
  {
    numVertices_line += 2 * (splines[i].numControlPoints - 2) * numSteps;
  }

  // Vertex positions
  positions = (float*) malloc (numVertices_line * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z

  // Vertex colors
  colors = (float*) malloc (numVertices_line * 4 * sizeof(float)); // 4 floats per vertex, i.e., r,g,b,a

  // First, allocate an empty VBO of the correct size to hold positions and colors.
  numBytesInPositions = numVertices_line * 3 * sizeof(float);
  numBytesInColors = numVertices_line * 4 * sizeof(float);

  int pos_index = 0; // index corresponding to 1D array positions
  int color_index = 0; // index corresponding to 1D array colors

  float s = 0.5f; // s = 1/2 for Catmull-Rom Spline Matrix
  glm::mat3x4 C(0.0f); // Control matrix (4x3 matrix)
  glm::mat4x4 M(-s,  2*s,   -s,   0.0f,
                2-s, s-3,   0.0f, 1.0f,
                s-2, 3-2*s, s,    0.0f,
                s,   -s,    0.0f, 0.0f); // Spline basis matrix (4x4 matrix)
  glm::mat3x4 MC(0.0f); // Matrix for M * C multiplication

  // loop over each spline
  for(int i = 0; i < numSplines; i++)
  {
    cout << "Spline " << i << endl;

    // loop over each control points in a spline
    for(int j = 1; j < splines[i].numControlPoints-1; j++)
    {
      cout << "x: " << splines[i].points[j].x << ", y: " << splines[i].points[j].y << ", z: " << splines[i].points[j].z << endl;

      // Assigning values to C matrix based on 4 control points
      for(int k = 0; k < 4; k++)
      {
        C[0][k] = (float)splines[i].points[j-(1-k)].x;
        C[1][k] = (float)splines[i].points[j-(1-k)].y;
        C[2][k] = (float)splines[i].points[j-(1-k)].z;
      }

      MC = M * C;

      // loop over all u values
      for(float u = 0.0f; u <= 1.0f; u += u_step)
      {
        u_vec[0] = pow(u,3);
        u_vec[1] = pow(u,2);
        u_vec[2] = u;

        pos_temp = u_vec * MC + initPos;

        // if we are at the beginning or the end of the loop, add the point in the positions vector once
        if( (u == 0.0f && (j == 1 || j == splines[i].numControlPoints-2)) || (u == 1.0f && j == splines[i].numControlPoints-2) )
        {
          // Vertex positions
          positions[pos_index++] = pos_temp[0]; // x position
          positions[pos_index++] = pos_temp[1]; // y position - height
          positions[pos_index++] = pos_temp[2]; // z position

          // Vertex colors
          colors[color_index++] = 1.0f;
          colors[color_index++] = 1.0f;
          colors[color_index++] = 1.0f;
          colors[color_index++] = 1.0f;
        }
        else
        {
          // Vertex positions
          positions[pos_index++] = pos_temp[0]; // x position
          positions[pos_index++] = pos_temp[1]; // y position - height
          positions[pos_index++] = pos_temp[2]; // z position

          positions[pos_index++] = pos_temp[0]; // x position
          positions[pos_index++] = pos_temp[1]; // y position - height
          positions[pos_index++] = pos_temp[2]; // z position    

          // Vertex colors
          colors[color_index++] = 1.0f;
          colors[color_index++] = 1.0f;
          colors[color_index++] = 1.0f;
          colors[color_index++] = 1.0f;     

          colors[color_index++] = 1.0f;
          colors[color_index++] = 1.0f;
          colors[color_index++] = 1.0f;
          colors[color_index++] = 1.0f;    
        }
      }
    }

    initPos = pos_temp; // update the initial position of the spline
  }

  // Create the VBOs. There is a single VBO in this example. This operation must be performed BEFORE we initialize any VAOs.
  glGenBuffers(1, &lineVBO);
  glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
  // First, allocate an empty VBO of the correct size to hold positions and colors.
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions + numBytesInColors, nullptr, GL_STATIC_DRAW);
  // Next, write the position and color data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositions, positions); // The VBO starts with positions.
  glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositions, numBytesInColors, colors); // The colors are written after the positions.

  // Create the VAOs. There is a single VAO in this example.
  glGenVertexArrays(1, &lineVAO);
  glBindVertexArray(lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, lineVBO); // The VBO that we bind here will be used in the glVertexAttribPointer calls below. If we forget to bind the VBO here, the program will malfunction.

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint line_locationOfPosition = glGetAttribLocation(linePipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(line_locationOfPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void *)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  // Set up the relationship between the "color" shader variable and the VAO.
  const GLuint line_locationOfColor = glGetAttribLocation(linePipelineProgram->GetProgramHandle(), "color"); // Obtain a handle to the shader variable "color".
  glEnableVertexAttribArray(line_locationOfColor); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfColor, 4, GL_FLOAT, normalized, stride, (const void *)(unsigned long)numBytesInPositions); // The shader variable "color" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset "numBytesInPositions" in the VBO. There are 4 float entries per vertex in the VBO (i.e., r,g,b,a channels). 

  // We don't need this data any more, as we have already uploaded it to the VBO. And so we can destroy it, to avoid a memory leak.
  free(positions);
  free(colors);

}

// prepare the sky box cube texture
void prepareCubeSkyBox()
{
  // create an integer handle for the texture - negative x
  glGenTextures(1, &skyCubeTexHandle_nx);
  int code = initTexture("textures/CubeMap1024/nx.jpg", skyCubeTexHandle_nx);
  if (code != 0)
  {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }

  // create an integer handle for the texture - negative y
  glGenTextures(1, &skyCubeTexHandle_ny);
  code = initTexture("textures/CubeMap1024/ny.jpg", skyCubeTexHandle_ny);
  if (code != 0)
  {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }
  
  // create an integer handle for the texture - negative z
  glGenTextures(1, &skyCubeTexHandle_nz);
  code = initTexture("textures/CubeMap1024/nz.jpg", skyCubeTexHandle_nz);
  if (code != 0)
  {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }
  
  // create an integer handle for the texture - positive x
  glGenTextures(1, &skyCubeTexHandle_px);
  code = initTexture("textures/CubeMap1024/px.jpg", skyCubeTexHandle_px);
  if (code != 0)
  {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }
  
  // create an integer handle for the texture - positive y
  glGenTextures(1, &skyCubeTexHandle_py);
  code = initTexture("textures/CubeMap1024/py.jpg", skyCubeTexHandle_py);
  if (code != 0)
  {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }
  
  // create an integer handle for the texture - positive z
  glGenTextures(1, &skyCubeTexHandle_pz);
  code = initTexture("textures/CubeMap1024/pz.jpg", skyCubeTexHandle_pz);
  if (code != 0)
  {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }

  vector <glm::vec3> texPos_nx; 
  vector <glm::vec3> texPos_ny;
  vector <glm::vec3> texPos_nz; 
  vector <glm::vec3> texPos_px; 
  vector <glm::vec3> texPos_py;
  vector <glm::vec3> texPos_pz;

  vector <glm::vec2> texUV_nx; 
  vector <glm::vec2> texUV_ny;
  vector <glm::vec2> texUV_nz; 
  vector <glm::vec2> texUV_px; 
  vector <glm::vec2> texUV_py;
  vector <glm::vec2> texUV_pz;

  // distance of the cube sky box
  float distSkyCubeTex = 100.0f;

  // vertices of a sky cube where z is up, and xy is ground
  vector <glm::vec3> vertices;
  vertices.push_back(glm::vec3(1,1,-1));    // 0
  vertices.push_back(glm::vec3(1,1,1));     // 1
  vertices.push_back(glm::vec3(1,-1,1));    // 2
  vertices.push_back(glm::vec3(1,-1,-1));   // 3
  vertices.push_back(glm::vec3(-1,1,-1));   // 4
  vertices.push_back(glm::vec3(-1,1,1));    // 5
  vertices.push_back(glm::vec3(-1,-1,1));   // 6
  vertices.push_back(glm::vec3(-1,-1,-1));  // 7

  // Positions and texture coordinates of cube sky box
  // negative x - vertices of the square: 4, 5, 6, 7
  // negative x - triangle 1
  texPos_nx.push_back(distSkyCubeTex * vertices[4]);
  texPos_nx.push_back(distSkyCubeTex * vertices[5]);
  texPos_nx.push_back(distSkyCubeTex * vertices[6]);
  texUV_nx.push_back(glm::vec2(1.0f,0.0f));
  texUV_nx.push_back(glm::vec2(1.0f,1.0f));
  texUV_nx.push_back(glm::vec2(0.0f,1.0f));
  // negative x - triangle 2
  texPos_nx.push_back(distSkyCubeTex * vertices[4]);
  texPos_nx.push_back(distSkyCubeTex * vertices[6]);
  texPos_nx.push_back(distSkyCubeTex * vertices[7]);
  texUV_nx.push_back(glm::vec2(1.0f,0.0f));
  texUV_nx.push_back(glm::vec2(0.0f,1.0f));
  texUV_nx.push_back(glm::vec2(0.0f,0.0f));

  // negative y - vertices of the square: 7, 6, 2, 3 
  // negative y - triangle 1
  texPos_ny.push_back(distSkyCubeTex * vertices[7]);
  texPos_ny.push_back(distSkyCubeTex * vertices[6]);
  texPos_ny.push_back(distSkyCubeTex * vertices[2]);
  texUV_ny.push_back(glm::vec2(1.0f,0.0f));
  texUV_ny.push_back(glm::vec2(1.0f,1.0f));
  texUV_ny.push_back(glm::vec2(0.0f,1.0f));
  // negative y - triangle 2
  texPos_ny.push_back(distSkyCubeTex * vertices[7]);
  texPos_ny.push_back(distSkyCubeTex * vertices[2]);
  texPos_ny.push_back(distSkyCubeTex * vertices[3]);
  texUV_ny.push_back(glm::vec2(1.0f,0.0f));
  texUV_ny.push_back(glm::vec2(0.0f,1.0f));
  texUV_ny.push_back(glm::vec2(0.0f,0.0f));

  // negative z - vertices of the square: 0, 4, 7, 3
  // negative z - triangle 1
  texPos_nz.push_back(distSkyCubeTex * vertices[0]);
  texPos_nz.push_back(distSkyCubeTex * vertices[4]);
  texPos_nz.push_back(distSkyCubeTex * vertices[7]);
  texUV_nz.push_back(glm::vec2(1.0f,0.0f));
  texUV_nz.push_back(glm::vec2(1.0f,1.0f));
  texUV_nz.push_back(glm::vec2(0.0f,1.0f));
  // negative z - triangle 2
  texPos_nz.push_back(distSkyCubeTex * vertices[0]);
  texPos_nz.push_back(distSkyCubeTex * vertices[7]);
  texPos_nz.push_back(distSkyCubeTex * vertices[3]);
  texUV_nz.push_back(glm::vec2(1.0f,0.0f));
  texUV_nz.push_back(glm::vec2(0.0f,1.0f));
  texUV_nz.push_back(glm::vec2(0.0f,0.0f));

  // positive x - vertices of the square: 3, 2, 1, 0
  // positive x - triangle 1
  texPos_px.push_back(distSkyCubeTex * vertices[3]);
  texPos_px.push_back(distSkyCubeTex * vertices[2]);
  texPos_px.push_back(distSkyCubeTex * vertices[1]);
  texUV_px.push_back(glm::vec2(1.0f,0.0f));
  texUV_px.push_back(glm::vec2(1.0f,1.0f));
  texUV_px.push_back(glm::vec2(0.0f,1.0f));
  // positive x - triangle 2
  texPos_px.push_back(distSkyCubeTex * vertices[3]);
  texPos_px.push_back(distSkyCubeTex * vertices[1]);
  texPos_px.push_back(distSkyCubeTex * vertices[0]);
  texUV_px.push_back(glm::vec2(1.0f,0.0f));
  texUV_px.push_back(glm::vec2(0.0f,1.0f));
  texUV_px.push_back(glm::vec2(0.0f,0.0f));

  // positive y - vertices of the square: 0, 1, 5, 4
  // positive y - triangle 1
  texPos_py.push_back(distSkyCubeTex * vertices[0]);
  texPos_py.push_back(distSkyCubeTex * vertices[1]);
  texPos_py.push_back(distSkyCubeTex * vertices[5]);
  texUV_py.push_back(glm::vec2(1.0f,0.0f));
  texUV_py.push_back(glm::vec2(1.0f,1.0f));
  texUV_py.push_back(glm::vec2(0.0f,1.0f));
  // positive y - triangle 2
  texPos_py.push_back(distSkyCubeTex * vertices[0]);
  texPos_py.push_back(distSkyCubeTex * vertices[5]);
  texPos_py.push_back(distSkyCubeTex * vertices[4]);
  texUV_py.push_back(glm::vec2(1.0f,0.0f));
  texUV_py.push_back(glm::vec2(0.0f,1.0f));
  texUV_py.push_back(glm::vec2(0.0f,0.0f));

  // positive z - vertices of the square: 2, 6, 5, 1
  // positive z - triangle 1
  texPos_pz.push_back(distSkyCubeTex * vertices[2]);
  texPos_pz.push_back(distSkyCubeTex * vertices[6]);
  texPos_pz.push_back(distSkyCubeTex * vertices[5]);
  texUV_pz.push_back(glm::vec2(1.0f,0.0f));
  texUV_pz.push_back(glm::vec2(1.0f,1.0f));
  texUV_pz.push_back(glm::vec2(0.0f,1.0f));
  // positive z - triangle 2
  texPos_pz.push_back(distSkyCubeTex * vertices[2]);
  texPos_pz.push_back(distSkyCubeTex * vertices[5]);
  texPos_pz.push_back(distSkyCubeTex * vertices[1]);
  texUV_pz.push_back(glm::vec2(1.0f,0.0f));
  texUV_pz.push_back(glm::vec2(0.0f,1.0f));
  texUV_pz.push_back(glm::vec2(0.0f,0.0f));

  numVertices_skyCubeTex = texPos_nx.size();

  int numBytesInPositions = sizeof(glm::vec3) * numVertices_skyCubeTex;
  int numBytesInTexCoords = sizeof(glm::vec2) * texUV_nx.size();

  GLsizei stride = 0;

  // --------------- Negative X ------------------
  // Create the VBO for negative x
  glGenBuffers(1, &skyCubeTexVBO_nx);
  glBindBuffer(GL_ARRAY_BUFFER, skyCubeTexVBO_nx);
  // Allocate an empty VBO of the correct size to hold positions and normals.
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions + numBytesInTexCoords, nullptr, GL_STATIC_DRAW);
  // Write the position and normals data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositions, texPos_nx.data()); // The VBO starts with positions.
  glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositions, numBytesInTexCoords, texUV_nx.data()); // The colors are written after the positions.

  // Create the VAO for negative x
  glGenVertexArrays(1, &skyCubeTexVAO_nx);
  glBindVertexArray(skyCubeTexVAO_nx);
  glBindBuffer(GL_ARRAY_BUFFER, skyCubeTexVBO_nx); // The VBO that we bind here will be used in the glVertexAttribPointer calls below. If we forget to bind the VBO here, the program will malfunction.

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint line_locationOfTexPosition_nx = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(line_locationOfTexPosition_nx); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfTexPosition_nx, 3, GL_FLOAT, GL_FALSE, stride, (const void *)0); // The shader variable "position" 

  // Set up the relationship between the "color" shader variable and the VAO.
  const GLuint line_locationOfTexCoord_nx = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord"); // Obtain a handle to the shader variable "texCoord".
  glEnableVertexAttribArray(line_locationOfTexCoord_nx); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfTexCoord_nx, 2, GL_FLOAT, GL_FALSE, stride, (const void *)(unsigned long)numBytesInPositions); // The shader variable "color" 

  // --------------- Negative Y ------------------
  // Create the VBO for negative y
  glGenBuffers(1, &skyCubeTexVBO_ny);
  glBindBuffer(GL_ARRAY_BUFFER, skyCubeTexVBO_ny);
  // Allocate an empty VBO of the correct size to hold positions and normals.
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions + numBytesInTexCoords, nullptr, GL_STATIC_DRAW);
  // Write the position and normals data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositions, texPos_ny.data()); // The VBO starts with positions.
  glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositions, numBytesInTexCoords, texUV_ny.data()); // The colors are written after the positions.

  // Create the VAO for negative y
  glGenVertexArrays(1, &skyCubeTexVAO_ny);
  glBindVertexArray(skyCubeTexVAO_ny);
  glBindBuffer(GL_ARRAY_BUFFER, skyCubeTexVBO_ny); // The VBO that we bind here will be used in the glVertexAttribPointer calls below. If we forget to bind the VBO here, the program will malfunction.

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint line_locationOfTexPosition_ny = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(line_locationOfTexPosition_ny); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfTexPosition_ny, 3, GL_FLOAT, GL_FALSE, stride, (const void *)0); // The shader variable "position" 

  // Set up the relationship between the "color" shader variable and the VAO.
  const GLuint line_locationOfTexCoord_ny = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord"); // Obtain a handle to the shader variable "texCoord".
  glEnableVertexAttribArray(line_locationOfTexCoord_ny); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfTexCoord_ny, 2, GL_FLOAT, GL_FALSE, stride, (const void *)(unsigned long)numBytesInPositions); // The shader variable "color" 

  // --------------- Negative Z ------------------
  // Create the VBO for negative z
  glGenBuffers(1, &skyCubeTexVBO_nz);
  glBindBuffer(GL_ARRAY_BUFFER, skyCubeTexVBO_nz);
  // Allocate an empty VBO of the correct size to hold positions and normals.
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions + numBytesInTexCoords, nullptr, GL_STATIC_DRAW);
  // Write the position and normals data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositions, texPos_nz.data()); // The VBO starts with positions.
  glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositions, numBytesInTexCoords, texUV_nz.data()); // The colors are written after the positions.

  // Create the VAO for negative z
  glGenVertexArrays(1, &skyCubeTexVAO_nz);
  glBindVertexArray(skyCubeTexVAO_nz);
  glBindBuffer(GL_ARRAY_BUFFER, skyCubeTexVBO_nz); // The VBO that we bind here will be used in the glVertexAttribPointer calls below. If we forget to bind the VBO here, the program will malfunction.

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint line_locationOfTexPosition_nz = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(line_locationOfTexPosition_nz); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfTexPosition_nz, 3, GL_FLOAT, GL_FALSE, stride, (const void *)0); // The shader variable "position" 

  // Set up the relationship between the "color" shader variable and the VAO.
  const GLuint line_locationOfTexCoord_nz = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord"); // Obtain a handle to the shader variable "texCoord".
  glEnableVertexAttribArray(line_locationOfTexCoord_nz); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfTexCoord_nz, 2, GL_FLOAT, GL_FALSE, stride, (const void *)(unsigned long)numBytesInPositions); // The shader variable "color" 

  // --------------- Positive X ------------------
  // Create the VBO for positive x
  glGenBuffers(1, &skyCubeTexVBO_px);
  glBindBuffer(GL_ARRAY_BUFFER, skyCubeTexVBO_px);
  // Allocate an empty VBO of the correct size to hold positions and normals.
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions + numBytesInTexCoords, nullptr, GL_STATIC_DRAW);
  // Write the position and normals data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositions, texPos_px.data()); // The VBO starts with positions.
  glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositions, numBytesInTexCoords, texUV_px.data()); // The colors are written after the positions.

  // Create the VAO for positive x
  glGenVertexArrays(1, &skyCubeTexVAO_px);
  glBindVertexArray(skyCubeTexVAO_px);
  glBindBuffer(GL_ARRAY_BUFFER, skyCubeTexVBO_px); // The VBO that we bind here will be used in the glVertexAttribPointer calls below. If we forget to bind the VBO here, the program will malfunction.

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint line_locationOfTexPosition_px = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(line_locationOfTexPosition_px); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfTexPosition_px, 3, GL_FLOAT, GL_FALSE, stride, (const void *)0); // The shader variable "position" 

  // Set up the relationship between the "color" shader variable and the VAO.
  const GLuint line_locationOfTexCoord_px = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord"); // Obtain a handle to the shader variable "texCoord".
  glEnableVertexAttribArray(line_locationOfTexCoord_px); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfTexCoord_px, 2, GL_FLOAT, GL_FALSE, stride, (const void *)(unsigned long)numBytesInPositions); // The shader variable "color" 

  // --------------- Positive Y ------------------
  // Create the VBO for positive y
  glGenBuffers(1, &skyCubeTexVBO_py);
  glBindBuffer(GL_ARRAY_BUFFER, skyCubeTexVBO_py);
  // Allocate an empty VBO of the correct size to hold positions and normals.
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions + numBytesInTexCoords, nullptr, GL_STATIC_DRAW);
  // Write the position and normals data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositions, texPos_py.data()); // The VBO starts with positions.
  glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositions, numBytesInTexCoords, texUV_py.data()); // The colors are written after the positions.

  // Create the VAO for positive y
  glGenVertexArrays(1, &skyCubeTexVAO_py);
  glBindVertexArray(skyCubeTexVAO_py);
  glBindBuffer(GL_ARRAY_BUFFER, skyCubeTexVBO_py); // The VBO that we bind here will be used in the glVertexAttribPointer calls below. If we forget to bind the VBO here, the program will malfunction.

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint line_locationOfTexPosition_py = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(line_locationOfTexPosition_py); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfTexPosition_py, 3, GL_FLOAT, GL_FALSE, stride, (const void *)0); // The shader variable "position" 

  // Set up the relationship between the "color" shader variable and the VAO.
  const GLuint line_locationOfTexCoord_py = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord"); // Obtain a handle to the shader variable "texCoord".
  glEnableVertexAttribArray(line_locationOfTexCoord_py); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfTexCoord_py, 2, GL_FLOAT, GL_FALSE, stride, (const void *)(unsigned long)numBytesInPositions); // The shader variable "color" 

  // --------------- Positive Z ------------------
  // Create the VBO for positive z
  glGenBuffers(1, &skyCubeTexVBO_pz);
  glBindBuffer(GL_ARRAY_BUFFER, skyCubeTexVBO_pz);
  // Allocate an empty VBO of the correct size to hold positions and normals.
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions + numBytesInTexCoords, nullptr, GL_STATIC_DRAW);
  // Write the position and normals data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositions, texPos_pz.data()); // The VBO starts with positions.
  glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositions, numBytesInTexCoords, texUV_pz.data()); // The colors are written after the positions.

  // Create the VAO for positive z
  glGenVertexArrays(1, &skyCubeTexVAO_pz);
  glBindVertexArray(skyCubeTexVAO_pz);
  glBindBuffer(GL_ARRAY_BUFFER, skyCubeTexVBO_pz); // The VBO that we bind here will be used in the glVertexAttribPointer calls below. If we forget to bind the VBO here, the program will malfunction.

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint line_locationOfTexPosition_pz = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(line_locationOfTexPosition_pz); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfTexPosition_pz, 3, GL_FLOAT, GL_FALSE, stride, (const void *)0); // The shader variable "position" 

  // Set up the relationship between the "color" shader variable and the VAO.
  const GLuint line_locationOfTexCoord_pz = glGetAttribLocation(texturePipelineProgram->GetProgramHandle(), "texCoord"); // Obtain a handle to the shader variable "texCoord".
  glEnableVertexAttribArray(line_locationOfTexCoord_pz); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfTexCoord_pz, 2, GL_FLOAT, GL_FALSE, stride, (const void *)(unsigned long)numBytesInPositions); // The shader variable "color" 

}


void initScene(int argc, char *argv[])
{
  // initialize the time
  oldTime = glutGet(GLUT_ELAPSED_TIME); // milliseconds
  frameCounter = 0;

  // Set the background color.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.

  // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
  glEnable(GL_DEPTH_TEST);

  // Create and bind the pipeline program. This operation must be performed BEFORE we initialize any VAOs.
  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) 
  { 
    abort();
  }
  pipelineProgram->Bind();

  // Create and bind the pipeline program. This operation must be performed BEFORE we initialize any VAOs.
  linePipelineProgram = new LinePipelineProgram;
  ret = linePipelineProgram->Init(shaderBasePath);
  if (ret != 0) 
  { 
    abort();
  }
  linePipelineProgram->Bind();

  // Send color change information to the shader
  colorChangeEnable = 0; // 0 means grayscale
  const GLuint locationOfColorEnable = glGetUniformLocation(linePipelineProgram->GetProgramHandle(), "colorChangeEnable"); // Obtain a handle to the shader variable "colorScale".
  glUniform1i(locationOfColorEnable, colorChangeEnable);

  // Send mode information to the shader
  const GLuint locationOfShaderMode = glGetUniformLocation(linePipelineProgram->GetProgramHandle(), "mode"); // Obtain a handle to the shader variable "mode".
  glUniform1i(locationOfShaderMode, 0);

  // Calculate spline positions, tangents, normals, binormals
  calculateSplineProperties();

  // Prepare only the splines to shown in the line mode
  linePipelineProgram->Bind();
  prepareSplinesLineMode(); // This is used in the Milestone 1

  // Prepare double rail road tracks
  pipelineProgram->Bind();
  prepareRailRoad();

  // Create and bind the pipeline program. This operation must be performed BEFORE we initialize any VAOs.
  texturePipelineProgram = new TexturePipelineProgram;
  ret = texturePipelineProgram->Init(shaderBasePath);
  if (ret != 0) 
  { 
    abort();
  }
  texturePipelineProgram->Bind();

  // Prepare the cube sky box texture
  prepareCubeSkyBox();

  // Check for any OpenGL errors.
  std::cout << "GL error: " << glGetError() << std::endl;
}


// Note for Windows/MS Visual Studio:
// You should set argv[1] to track.txt.
// To do this, on the "Solution Explorer",
// right click your project, choose "Properties",
// go to "Configuration Properties", click "Debug",
// then type your track file name for the "Command Arguments".
// You can also repeat this process for the "Release" configuration.

int main(int argc, char ** argv)
{
  if (argc<2)
  {  
    printf ("usage: %s <trackfile>\n", argv[0]);
    exit(0);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
  #endif

  // tells glut to use a particular display function to redraw 
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // load the splines from the provided filename
  loadSplines(argv[1]);

  printf("Loaded %d spline(s).\n", numSplines);
  for(int i=0; i<numSplines; i++)
    printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);

  initScene(argc, argv);

  // Sink forever into the GLUT loop.
  glutMainLoop();
}


