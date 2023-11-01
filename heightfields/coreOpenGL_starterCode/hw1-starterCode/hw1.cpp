/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields with Shaders.
  C++ starter code

*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include "math.h"

#include <iostream>
#include <cstring>

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

int mousePos[2]; // x,y screen coordinates of the current mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

typedef enum { POINTS, LINES, TRIANGLES, SMOOTH } RENDER_MODE;
RENDER_MODE renderMode = POINTS;

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
char windowTitle[512] = "CSCI 420 homework I";

// Stores the image loaded from disk.
ImageIO * heightmapImage;

// VBO and VAO handles.
GLuint pointVBO;
GLuint lineVBO;
GLuint triangleVBO;

GLuint centerVBO;
GLuint leftVBO;
GLuint rightVBO;
GLuint upVBO;
GLuint downVBO;

GLuint pointVAO;
GLuint lineVAO;
GLuint triangleVAO;
GLuint smoothVAO;

int numVertices_point;
int numVertices_line;
int numVertices_triangle;
int numVertices_smooth;

int colorChangeEnable; // enable and disable coloring
bool colorFreeze; // freeze the color change if f is pressed, contionue changing color if c is pressed

bool autoSave; // control for starting automatic screenshot save, run if s is pressed
float curTime;
float oldTime;
float deltaTime;
int frameRatePerSecond = 15;
int numFrames = 300;
int frameCounter;

string tmpCounter;
char ssName[3];
const char *tmpchar = NULL;


// CSCI 420 helper classes.
OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;

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
  matrix.LookAt(0.0, 70.0, -15.0, 
                0.0, 50.0, 0.0, 
                0.0, 1.0, 0.0);

  // In here, you can do additional modeling on the object, such as performing translations, rotations and scales.
  // ...

  matrix.Translate(terrainTranslate[0],terrainTranslate[1],terrainTranslate[2]);
  matrix.Rotate(terrainRotate[0],1,0,0);
  matrix.Rotate(terrainRotate[1],0,1,0);
  matrix.Rotate(terrainRotate[2],0,0,1);
  matrix.Scale(terrainScale[0],terrainScale[1],terrainScale[2]);

  // Read the current modelview and projection matrices.
  float modelViewMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(modelViewMatrix);

  float projectionMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(projectionMatrix);

  // Bind the pipeline program.
  // If an incorrect pipeline program is bound, then the modelview and projection matrices
  // will be sent to the wrong pipeline program, causing shader 
  // malfunction (typically, nothing will be shown on the screen).
  // In this homework, there is only one pipeline program, and it is already bound.
  // So technically speaking, this call is redundant in hw1.
  // However, in more complex programs (such as hw2), there will be more than one
  // pipeline program. And so we will need to bind the pipeline program that we want to use.
  pipelineProgram->Bind(); // This call is redundant in hw1, but it is good to keep for consistency.

  // Upload the modelview and projection matrices to the GPU.
  pipelineProgram->SetModelViewMatrix(modelViewMatrix);
  pipelineProgram->SetProjectionMatrix(projectionMatrix);

  switch (renderMode)
  {
  case POINTS:
    // Execute the rendering.
    glBindVertexArray(pointVAO); // Bind the VAO that we want to render.
    glDrawArrays(GL_POINTS, 0, numVertices_point); // Render the VAO, by rendering "numVertices", starting from vertex 0.

    break;

  case LINES:
    // Execute the rendering.
    glBindVertexArray(lineVAO); // Bind the VAO that we want to render.
    glDrawArrays(GL_LINES, 0, numVertices_line); // Render the VAO, by rendering "numVertices", starting from vertex 0.

    break;

  case TRIANGLES:
    // Execute the rendering.
    glBindVertexArray(triangleVAO); // Bind the VAO that we want to render.
    glDrawArrays(GL_TRIANGLES, 0, numVertices_triangle); // Render the VAO, by rendering "numVertices", starting from vertex 0.

    break;

  case SMOOTH:
    // Execute the rendering.
    glBindVertexArray(smoothVAO); // Bind the VAO that we want to render.
    glDrawArrays(GL_TRIANGLES, 0, numVertices_smooth); // Render the VAO, by rendering "numVertices", starting from vertex 0.

    break;    
  
  }

  // Swap the double-buffers.
  glutSwapBuffers();
}

void idleFunc()
{

  // get the current time elpased
  curTime = glutGet(GLUT_ELAPSED_TIME); // milliseconds
  deltaTime += (curTime - oldTime)/1000; // delta time in seconds
  oldTime = curTime;

  // if autosave is pressed save screenshot until we reach numFrames (which is 300)
  if( autoSave )
  {
    if(frameCounter == numFrames)
    {
      autoSave = false;
      frameCounter = 0;
    }
    while(deltaTime * frameRatePerSecond >= 1)
    {
      deltaTime = 0;
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
  const GLuint locationOfMouseX = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mouseX"); // Obtain a handle to the shader variable "colorScale".
  const GLuint locationOfMouseY = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mouseY"); // Obtain a handle to the shader variable "colorScale".
 
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
  const GLuint locationOfShaderMode = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode"); // Obtain a handle to the shader variable "mode".
  const GLuint locationOfColorEnable = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "colorChangeEnable"); // Obtain a handle to the shader variable "colorScale".
  
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

    // Translate when w is pressed (CTRL and ALT did not work with Mac)
    case 'w':
      controlState = TRANSLATE;
    break;

    // Render Mode is Points when key '1' is pressed, Shader mode is 0
    case '1':
      renderMode = POINTS;
      glUniform1i(locationOfShaderMode, 0);
      colorChangeEnable = 0;
      glUniform1i(locationOfColorEnable, colorChangeEnable);
    break;

    // Render Mode is Lines when key '2' is pressed
    case '2':
      renderMode = LINES;
      glUniform1i(locationOfShaderMode, 0);
      colorChangeEnable = 0;
      glUniform1i(locationOfColorEnable, colorChangeEnable);
    break;

    // Render Mode is Triangles when key '3' is pressed
    case '3':
      renderMode = TRIANGLES;
      glUniform1i(locationOfShaderMode, 0);
      colorChangeEnable = 0;
      glUniform1i(locationOfColorEnable, colorChangeEnable);
    break;

    // Render Mode is Smoothing when key '4' is pressed
    case '4':
      renderMode = SMOOTH;
      glUniform1i(locationOfShaderMode, 1);
      colorChangeEnable = 0;
      glUniform1i(locationOfColorEnable, colorChangeEnable);
    break;

    case '5':
      colorChangeEnable = 1;
      glUniform1i(locationOfColorEnable, colorChangeEnable);
    break;

    case 'f':
      colorFreeze = true;
    break;

    case 'c':
      colorFreeze = false;
    break;
  }
}

void initScene(int argc, char *argv[])
{
  // initialize the time
  oldTime = glutGet(GLUT_ELAPSED_TIME); // milliseconds
  frameCounter = 0;

  // Load the image from a jpeg disk file into main memory.
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }

  // Check the dimensions of the image
  cout << "image width: " << heightmapImage->getWidth() << ", height: " << heightmapImage->getHeight() << endl;
  
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

  // Send color change information to the shader
  colorChangeEnable = 0; // 0 means grayscale
  const GLuint locationOfColorEnable = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "colorChangeEnable"); // Obtain a handle to the shader variable "colorScale".
  glUniform1i(locationOfColorEnable, colorChangeEnable);

  // Send mode information to the shader
  const GLuint locationOfShaderMode = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode"); // Obtain a handle to the shader variable "mode".
  glUniform1i(locationOfShaderMode, 0);

  // Prepare the triangle position and color data for the VBO. 
  // The code below sets up a single triangle (3 vertices).
  // The triangle will be rendered using GL_TRIANGLES (in displayFunc()).

  // Get the dimensions of the image
  const int imageWidth = heightmapImage->getWidth();
  const int imageHeight = heightmapImage->getHeight();

  // Vertex positions
  float * positions;
  // Vertex colors
  float * colors;

  int numBytesInPositions; 
  int numBytesInColors;

  const int stride = 0; // Stride is 0, i.e., data is tightly packed in the VBO.
  const GLboolean normalized = GL_FALSE; // Normalization is off.

  float heightScale = 0.1;

  //-------------------- RENDER MODE: POINTS (Key '1') -----------------------

  numVertices_point = imageWidth * imageHeight;

  // Vertex positions
  float * point_positions = (float*) malloc (numVertices_point * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z

  // Vertex colors
  float * point_colors = (float*) malloc (numVertices_point * 4 * sizeof(float)); // 4 floats per vertex, i.e., r,g,b,a

  // First, allocate an empty VBO of the correct size to hold positions and colors.
  numBytesInPositions = numVertices_point * 3 * sizeof(float);
  numBytesInColors = numVertices_point * 4 * sizeof(float);

  float height = 0.0; // temporary variable to save the height of the current pixel

  int index = 0; // index corresponding to 1D array converted from 2D array (image) indices

  float maxHeight = 0.0; // maximum height of the terrain

  // loop over each pixel
  for(int i = 0; i < imageWidth; i++)
  {
    for(int j = 0; j < imageHeight; j++)
    {
      index = i * imageHeight + j;

      height = heightmapImage->getPixel(i, j, 0);

      // Vertex positions
      point_positions[index * 3] = i - imageWidth/2; // x position
      point_positions[index * 3 + 1] = heightScale * height; // y position - height
      point_positions[index * 3 + 2] = -j + imageHeight/2; // z position

      // Vertex colors
      point_colors[index * 4] = height/255;
      point_colors[index * 4 + 1] = height/255;
      point_colors[index * 4 + 2] = height/255;
      point_colors[index * 4 + 3] = 1.0;

      // update maximum height
      maxHeight = max(maxHeight,heightScale * height);
    }
  }

  // send maximum height to the shader
  const GLuint locationOfMaxHeight = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "maxHeight"); // Obtain a handle to the shader variable "maxHeight".
  glUniform1f(locationOfMaxHeight, maxHeight);

  // Create the VBOs. There is a single VBO in this example. This operation must be performed BEFORE we initialize any VAOs.
  glGenBuffers(1, &pointVBO);
  glBindBuffer(GL_ARRAY_BUFFER, pointVBO);

  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions + numBytesInColors, nullptr, GL_STATIC_DRAW);
  // Next, write the position and color data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositions, point_positions); // The VBO starts with positions.
  glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositions, numBytesInColors, point_colors); // The colors are written after the positions.

  // Create the VAOs. There is a single VAO in this example.
  glGenVertexArrays(1, &pointVAO);
  glBindVertexArray(pointVAO);
  glBindBuffer(GL_ARRAY_BUFFER, pointVBO); // The VBO that we bind here will be used in the glVertexAttribPointer calls below. If we forget to bind the VBO here, the program will malfunction.

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint point_locationOfPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(point_locationOfPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(point_locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void *)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  // Set up the relationship between the "color" shader variable and the VAO.
  const GLuint point_locationOfColor = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color"); // Obtain a handle to the shader variable "color".
  glEnableVertexAttribArray(point_locationOfColor); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(point_locationOfColor, 4, GL_FLOAT, normalized, stride, (const void *)(unsigned long)numBytesInPositions); // The shader variable "color" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset "numBytesInPositions" in the VBO. There are 4 float entries per vertex in the VBO (i.e., r,g,b,a channels). 

  // We don't need this data any more, as we have already uploaded it to the VBO. And so we can destroy it, to avoid a memory leak.
  free(point_positions);
  free(point_colors);

  //-------------------- RENDER MODE: LINES (Key '2') -----------------------

  numVertices_line = 4 * imageWidth * imageHeight - 2 * (imageWidth + imageHeight);

  // Vertex positions
  positions = (float*) malloc (numVertices_line * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z

  // Vertex colors
  colors = (float*) malloc (numVertices_line * 4 * sizeof(float)); // 4 floats per vertex, i.e., r,g,b,a

  // First, allocate an empty VBO of the correct size to hold positions and colors.
  numBytesInPositions = numVertices_line * 3 * sizeof(float);
  numBytesInColors = numVertices_line * 4 * sizeof(float);

  int pos_index = 0; // index corresponding to 1D array positions
  int color_index = 0; // index corresponding to 1D array colors

  // loop over each pixel
  for(int i = 0; i < imageWidth; i++)
  {
    for(int j = 0; j < imageHeight; j++)
    {

      if(i < imageWidth-1)
      {
        // ---- horizontal lines ----

        // current pixel
        height = heightmapImage->getPixel(i, j, 0);

        // Vertex positions of the first point of a line
        positions[pos_index++] = i - imageWidth/2; // x position
        positions[pos_index++] = heightScale * height; // y position - height
        positions[pos_index++] = -j + imageHeight/2; // z position

        // ---- Vertex colors ----
        colors[color_index++] = height/255;
        colors[color_index++] = height/255;
        colors[color_index++] = height/255;
        colors[color_index++] = 1.0;

        // right pixel
        height = heightmapImage->getPixel(i+1, j, 0);

        // Vertex positions of the second point of a line
        positions[pos_index++] = (i+1) - imageWidth/2; // x position
        positions[pos_index++] = heightScale * height; // y position - height
        positions[pos_index++] = -j + imageHeight/2; // z position

        // ---- Vertex colors ----
        colors[color_index++] = height/255;
        colors[color_index++] = height/255;
        colors[color_index++] = height/255;
        colors[color_index++] = 1.0;
      }

      if(j < imageHeight-1)
      {
        // ---- vertical lines ----

        // current pixel
        height = heightmapImage->getPixel(i, j, 0);

        // Vertex positions of the first point of a line
        positions[pos_index++] = i - imageWidth/2; // x position
        positions[pos_index++] = heightScale * height; // y position - height
        positions[pos_index++] = -j + imageHeight/2; // z position

        // ---- Vertex colors ----
        colors[color_index++] = height/255;
        colors[color_index++] = height/255;
        colors[color_index++] = height/255;
        colors[color_index++] = 1.0;

        // down pixel
        height = heightmapImage->getPixel(i, j+1, 0);

        // Vertex positions of the second point of a line
        positions[pos_index++] = i - imageWidth/2; // x position
        positions[pos_index++] = heightScale * height; // y position - height
        positions[pos_index++] = -(j+1) + imageHeight/2; // z position

        // ---- Vertex colors ----
        colors[color_index++] = height/255;
        colors[color_index++] = height/255;
        colors[color_index++] = height/255;
        colors[color_index++] = 1.0;
      }
    }
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
  const GLuint line_locationOfPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(line_locationOfPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void *)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  // Set up the relationship between the "color" shader variable and the VAO.
  const GLuint line_locationOfColor = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color"); // Obtain a handle to the shader variable "color".
  glEnableVertexAttribArray(line_locationOfColor); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(line_locationOfColor, 4, GL_FLOAT, normalized, stride, (const void *)(unsigned long)numBytesInPositions); // The shader variable "color" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset "numBytesInPositions" in the VBO. There are 4 float entries per vertex in the VBO (i.e., r,g,b,a channels). 

  // We don't need this data any more, as we have already uploaded it to the VBO. And so we can destroy it, to avoid a memory leak.
  free(positions);
  free(colors);

  //-------------------- RENDER MODE: TRIANGLES (Key '3') -----------------------

  numVertices_triangle = 6 * (imageWidth-1) * (imageHeight-1);

  // Vertex positions
  positions = (float*) malloc (numVertices_triangle * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z

  // Vertex colors
  colors = (float*) malloc (numVertices_triangle * 4 * sizeof(float)); // 4 floats per vertex, i.e., r,g,b,a

  // First, allocate an empty VBO of the correct size to hold positions and colors.
  numBytesInPositions = numVertices_triangle * 3 * sizeof(float);
  numBytesInColors = numVertices_triangle * 4 * sizeof(float);

  pos_index = 0; // index corresponding to 1D array positions
  color_index = 0; // index corresponding to 1D array colors

  // loop over each pixel
  for(int i = 0; i < imageWidth-1; i++)
  {
    for(int j = 0; j < imageHeight-1; j++)
    {

      // ------------------ lower triangle ---------------------

      // ------ current pixel (left & down) ------
      height = heightmapImage->getPixel(i, j, 0);

      // Vertex positions of the down left point in the lower triangle
      positions[pos_index++] = i - imageWidth/2; // x position
      positions[pos_index++] = heightScale * height; // y position - height
      positions[pos_index++] = -j + imageHeight/2; // z position

      // Vertex colors
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = 1.0;

      // ------ right & down pixel ------
      height = heightmapImage->getPixel(i+1, j, 0);

      // Vertex positions of the down right point in the lower triangle
      positions[pos_index++] = (i+1) - imageWidth/2; // x position
      positions[pos_index++] = heightScale * height; // y position - height
      positions[pos_index++] = -j + imageHeight/2; // z position

      // Vertex colors
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = 1.0;

      // ------ left & up pixel ------
      height = heightmapImage->getPixel(i, j+1, 0);

      // Vertex positions of the upper left point in the lower triangle
      positions[pos_index++] = i - imageWidth/2; // x position
      positions[pos_index++] = heightScale * height; // y position - height
      positions[pos_index++] = -(j+1) + imageHeight/2; // z position

      // Vertex colors
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = 1.0;

      // ------------------ upper triangle ---------------------

      // ------ right & up pixel ------
      height = heightmapImage->getPixel(i+1, j+1, 0);

      // Vertex positions of the upper right point in the upper triangle
      positions[pos_index++] = i+1 - imageWidth/2; // x position
      positions[pos_index++] = heightScale * height; // y position - height
      positions[pos_index++] = -(j+1) + imageHeight/2; // z position

      // Vertex colors 
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = 1.0;

      // ------ left & up pixel ------
      height = heightmapImage->getPixel(i, j+1, 0);

      // Vertex positions of the upper left point in the upper triangle
      positions[pos_index++] = i - imageWidth/2; // x position
      positions[pos_index++] = heightScale * height; // y position - height
      positions[pos_index++] = -(j+1) + imageHeight/2; // z position

      // Vertex colors 
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = 1.0;

      // ------ right & down pixel ------
      height = heightmapImage->getPixel(i+1, j, 0);

      // Vertex positions of the down right point in the upper triangle
      positions[pos_index++] = (i+1) - imageWidth/2; // x position
      positions[pos_index++] = heightScale * height; // y position - height
      positions[pos_index++] = -j + imageHeight/2; // z position

      // Vertex colors
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = 1.0;

      // Vertex colors
      // colors[color_index++] = fmodf(terrainRotate[0]+terrainRotate[1]+terrainRotate[2])/3,360.0)/360.0;
      // colors[color_index++] = height/255;
      // colors[color_index++] = 1.0;
      // colors[color_index++] = 1.0;

      // colors[color_index++] = 0.0;
      // colors[color_index++] = height/255;
      // colors[color_index++] = 1.0;
      // colors[color_index++] = 1.0;
    }
  }

  // Create the VBOs. There is a single VBO in this example. This operation must be performed BEFORE we initialize any VAOs.
  glGenBuffers(1, &triangleVBO);
  glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
  // First, allocate an empty VBO of the correct size to hold positions and colors.
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions + numBytesInColors, nullptr, GL_STATIC_DRAW);
  // Next, write the position and color data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositions, positions); // The VBO starts with positions.
  glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositions, numBytesInColors, colors); // The colors are written after the positions.

  // Create the VAOs. There is a single VAO in this example.
  glGenVertexArrays(1, &triangleVAO);
  glBindVertexArray(triangleVAO);
  glBindBuffer(GL_ARRAY_BUFFER, triangleVBO); // The VBO that we bind here will be used in the glVertexAttribPointer calls below. If we forget to bind the VBO here, the program will malfunction.

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint triangle_locationOfPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(triangle_locationOfPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(triangle_locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void *)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  // Set up the relationship between the "color" shader variable and the VAO.
  const GLuint triangle_locationOfColor = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color"); // Obtain a handle to the shader variable "color".
  glEnableVertexAttribArray(triangle_locationOfColor); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(triangle_locationOfColor, 4, GL_FLOAT, normalized, stride, (const void *)(unsigned long)numBytesInPositions); // The shader variable "color" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset "numBytesInPositions" in the VBO. There are 4 float entries per vertex in the VBO (i.e., r,g,b,a channels). 

  // We don't need this data any more, as we have already uploaded it to the VBO. And so we can destroy it, to avoid a memory leak.
  free(positions);
  free(colors);

  //-------------------- RENDER MODE: SMOOTH (Key '4') -----------------------

  numVertices_smooth = 6 * (imageWidth-1) * (imageHeight-1);

  // Vertex positions
  float * positions_center = (float*) malloc (numVertices_smooth * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z
  float * positions_left = (float*) malloc (numVertices_smooth * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z
  float * positions_right = (float*) malloc (numVertices_smooth * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z
  float * positions_down = (float*) malloc (numVertices_smooth * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z
  float * positions_up = (float*) malloc (numVertices_smooth * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z

  // Vertex colors
  colors = (float*) malloc (numVertices_smooth * 4 * sizeof(float)); // 4 floats per vertex, i.e., r,g,b,a

  // First, allocate an empty VBO of the correct size to hold positions and colors.
  numBytesInPositions = numVertices_smooth * 3 * sizeof(float);
  numBytesInColors = numVertices_smooth * 4 * sizeof(float);

  pos_index = 0; // index corresponding to 1D array positions
  color_index = 0; // index corresponding to 1D array colors

  int pos_index_left = 0; // index for left
  int pos_index_right = 0; // index for right
  int pos_index_down = 0; // index for down
  int pos_index_up = 0; // index for up

  int temp_i = 0; // temporary index i to handle the edge cases
  int temp_j = 0; // temporary index i to handle the edge cases

  int cur_i = 0; // current index i to navigate between vertices of the triangles inside each loop
  int cur_j = 0; // current index j to navigate between vertices of the triangles inside each loop

  // loop over each pixel
  for(int i = 0; i < imageWidth-1; i++)
  {
    for(int j = 0; j < imageHeight-1; j++)
    {

      // ------------------ lower triangle ---------------------

      // ------ current pixel (left & down) ------
      cur_i = i;
      cur_j = j;
      height = heightmapImage->getPixel(cur_i, cur_j, 0);

      // Vertex colors
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = 1.0;

      // Vertex positions of the down left point in the lower triangle
      positions_center[pos_index++] = cur_i - imageWidth/2; // x position
      positions_center[pos_index++] = heightScale * height; // y position - height
      positions_center[pos_index++] = -cur_j + imageHeight/2; // z position

      // left
      temp_i = max(cur_i-1,0); // replacing the left pos with the center pos if it is an edge
      height = heightmapImage->getPixel(temp_i, cur_j, 0);
      positions_left[pos_index_left++] = temp_i - imageWidth/2; // x position
      positions_left[pos_index_left++] = heightScale * height; // y position - height
      positions_left[pos_index_left++] = -cur_j + imageHeight/2; // z position

      // right
      temp_i = min(cur_i+1,imageWidth-1); // replacing the right pos with the center pos if it is an edge
      height = heightmapImage->getPixel(temp_i, cur_j, 0);
      positions_right[pos_index_right++] = temp_i - imageWidth/2; // x position
      positions_right[pos_index_right++] = heightScale * height; // y position - height
      positions_right[pos_index_right++] = -cur_j + imageHeight/2; // z position

      // down
      temp_j = max(cur_j-1,0); // replacing the down pos with the center pos if it is an edge
      height = heightmapImage->getPixel(cur_i, temp_j, 0);
      positions_down[pos_index_down++] = cur_i - imageWidth/2; // x position
      positions_down[pos_index_down++] = heightScale * height; // y position - height
      positions_down[pos_index_down++] = -temp_j + imageHeight/2; // z position

      // up
      temp_j = min(cur_j+1,imageHeight-1); // replacing the up pos with the center pos if it is an edge
      height = heightmapImage->getPixel(cur_i, temp_j, 0);
      positions_up[pos_index_up++] = cur_i - imageWidth/2; // x position
      positions_up[pos_index_up++] = heightScale * height; // y position - height
      positions_up[pos_index_up++] = -temp_j + imageHeight/2; // z position

      // ------ right & down pixel ------
      cur_i = i+1;
      cur_j = j;
      height = heightmapImage->getPixel(cur_i, cur_j, 0);

      // Vertex colors
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = 1.0;

      // Vertex positions of the down right point in the lower triangle
      positions_center[pos_index++] = cur_i - imageWidth/2; // x position
      positions_center[pos_index++] = heightScale * height; // y position - height
      positions_center[pos_index++] = -cur_j + imageHeight/2; // z position

      // left
      temp_i = max(cur_i-1,0); // replacing the left pos with the center pos if it is an edge
      height = heightmapImage->getPixel(temp_i, cur_j, 0);
      positions_left[pos_index_left++] = temp_i - imageWidth/2; // x position
      positions_left[pos_index_left++] = heightScale * height; // y position - height
      positions_left[pos_index_left++] = -cur_j + imageHeight/2; // z position

      // right
      temp_i = min(cur_i+1,imageWidth-1); // replacing the right pos with the center pos if it is an edge
      height = heightmapImage->getPixel(temp_i, cur_j, 0);
      positions_right[pos_index_right++] = temp_i - imageWidth/2; // x position
      positions_right[pos_index_right++] = heightScale * height; // y position - height
      positions_right[pos_index_right++] = -cur_j + imageHeight/2; // z position

      // down
      temp_j = max(cur_j-1,0); // replacing the down pos with the center pos if it is an edge
      height = heightmapImage->getPixel(cur_i, temp_j, 0);
      positions_down[pos_index_down++] = cur_i - imageWidth/2; // x position
      positions_down[pos_index_down++] = heightScale * height; // y position - height
      positions_down[pos_index_down++] = -temp_j + imageHeight/2; // z position

      // up
      temp_j = min(cur_j+1,imageHeight-1); // replacing the up pos with the center pos if it is an edge
      height = heightmapImage->getPixel(cur_i, temp_j, 0);
      positions_up[pos_index_up++] = cur_i - imageWidth/2; // x position
      positions_up[pos_index_up++] = heightScale * height; // y position - height
      positions_up[pos_index_up++] = -temp_j + imageHeight/2; // z position

      // ------ left & up pixel ------
      cur_i = i;
      cur_j = j+1;
      height = heightmapImage->getPixel(cur_i, cur_j, 0);

      // Vertex colors
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = 1.0;

      // Vertex positions of the upper left point in the lower triangle
      positions_center[pos_index++] = cur_i - imageWidth/2; // x position
      positions_center[pos_index++] = heightScale * height; // y position - height
      positions_center[pos_index++] = -cur_j + imageHeight/2; // z position

      // left
      temp_i = max(cur_i-1,0); // replacing the left pos with the center pos if it is an edge
      height = heightmapImage->getPixel(temp_i, cur_j, 0);
      positions_left[pos_index_left++] = temp_i - imageWidth/2; // x position
      positions_left[pos_index_left++] = heightScale * height; // y position - height
      positions_left[pos_index_left++] = -cur_j + imageHeight/2; // z position

      // right
      temp_i = min(cur_i+1,imageWidth-1); // replacing the right pos with the center pos if it is an edge
      height = heightmapImage->getPixel(temp_i, cur_j, 0);
      positions_right[pos_index_right++] = temp_i - imageWidth/2; // x position
      positions_right[pos_index_right++] = heightScale * height; // y position - height
      positions_right[pos_index_right++] = -cur_j + imageHeight/2; // z position

      // down
      temp_j = max(cur_j-1,0); // replacing the down pos with the center pos if it is an edge
      height = heightmapImage->getPixel(cur_i, temp_j, 0);
      positions_down[pos_index_down++] = cur_i - imageWidth/2; // x position
      positions_down[pos_index_down++] = heightScale * height; // y position - height
      positions_down[pos_index_down++] = -temp_j + imageHeight/2; // z position

      // up
      temp_j = min(cur_j+1,imageHeight-1); // replacing the up pos with the center pos if it is an edge
      height = heightmapImage->getPixel(cur_i, temp_j, 0);
      positions_up[pos_index_up++] = cur_i - imageWidth/2; // x position
      positions_up[pos_index_up++] = heightScale * height; // y position - height
      positions_up[pos_index_up++] = -temp_j + imageHeight/2; // z position

      // ------------------ upper triangle ---------------------

      // ------ right & up pixel ------
      cur_i = i+1;
      cur_j = j+1;
      height = heightmapImage->getPixel(cur_i, cur_j, 0);

      // Vertex colors
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = 1.0;

      // Vertex positions of the upper right point in the upper triangle
      positions_center[pos_index++] = cur_i - imageWidth/2; // x position
      positions_center[pos_index++] = heightScale * height; // y position - height
      positions_center[pos_index++] = -cur_j + imageHeight/2; // z position

      // left
      temp_i = max(cur_i-1,0); // replacing the left pos with the center pos if it is an edge
      height = heightmapImage->getPixel(temp_i, cur_j, 0);
      positions_left[pos_index_left++] = temp_i - imageWidth/2; // x position
      positions_left[pos_index_left++] = heightScale * height; // y position - height
      positions_left[pos_index_left++] = -cur_j + imageHeight/2; // z position

      // right
      temp_i = min(cur_i+1,imageWidth-1); // replacing the right pos with the center pos if it is an edge
      height = heightmapImage->getPixel(temp_i, cur_j, 0);
      positions_right[pos_index_right++] = temp_i - imageWidth/2; // x position
      positions_right[pos_index_right++] = heightScale * height; // y position - height
      positions_right[pos_index_right++] = -cur_j + imageHeight/2; // z position

      // down
      temp_j = max(cur_j-1,0); // replacing the down pos with the center pos if it is an edge
      height = heightmapImage->getPixel(cur_i, temp_j, 0);
      positions_down[pos_index_down++] = cur_i - imageWidth/2; // x position
      positions_down[pos_index_down++] = heightScale * height; // y position - height
      positions_down[pos_index_down++] = -temp_j + imageHeight/2; // z position

      // up
      temp_j = min(cur_j+1,imageHeight-1); // replacing the up pos with the center pos if it is an edge
      height = heightmapImage->getPixel(cur_i, temp_j, 0);
      positions_up[pos_index_up++] = cur_i - imageWidth/2; // x position
      positions_up[pos_index_up++] = heightScale * height; // y position - height
      positions_up[pos_index_up++] = -temp_j + imageHeight/2; // z position

      // ------ left & up pixel ------
      cur_i = i;
      cur_j = j+1;
      height = heightmapImage->getPixel(cur_i, cur_j, 0);

      // Vertex colors
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = 1.0;

      // Vertex positions of the upper left point in the upper triangle
      positions_center[pos_index++] = cur_i - imageWidth/2; // x position
      positions_center[pos_index++] = heightScale * height; // y position - height
      positions_center[pos_index++] = -cur_j + imageHeight/2; // z position

      // left
      temp_i = max(cur_i-1,0); // replacing the left pos with the center pos if it is an edge
      height = heightmapImage->getPixel(temp_i, cur_j, 0);
      positions_left[pos_index_left++] = temp_i - imageWidth/2; // x position
      positions_left[pos_index_left++] = heightScale * height; // y position - height
      positions_left[pos_index_left++] = -cur_j + imageHeight/2; // z position

      // right
      temp_i = min(cur_i+1,imageWidth-1); // replacing the right pos with the center pos if it is an edge
      height = heightmapImage->getPixel(temp_i, cur_j, 0);
      positions_right[pos_index_right++] = temp_i - imageWidth/2; // x position
      positions_right[pos_index_right++] = heightScale * height; // y position - height
      positions_right[pos_index_right++] = -cur_j + imageHeight/2; // z position

      // down
      temp_j = max(cur_j-1,0); // replacing the down pos with the center pos if it is an edge
      height = heightmapImage->getPixel(cur_i, temp_j, 0);
      positions_down[pos_index_down++] = cur_i - imageWidth/2; // x position
      positions_down[pos_index_down++] = heightScale * height; // y position - height
      positions_down[pos_index_down++] = -temp_j + imageHeight/2; // z position

      // up
      temp_j = min(cur_j+1,imageHeight-1); // replacing the up pos with the center pos if it is an edge
      height = heightmapImage->getPixel(cur_i, temp_j, 0);
      positions_up[pos_index_up++] = cur_i - imageWidth/2; // x position
      positions_up[pos_index_up++] = heightScale * height; // y position - height
      positions_up[pos_index_up++] = -temp_j + imageHeight/2; // z position

      // ------ right & down pixel ------
      cur_i = i+1;
      cur_j = j;
      height = heightmapImage->getPixel(cur_i, cur_j, 0);

      // Vertex colors
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = height/255;
      colors[color_index++] = 1.0;

      // Vertex positions of the down right point in the upper triangle
      positions_center[pos_index++] = cur_i - imageWidth/2; // x position
      positions_center[pos_index++] = heightScale * height; // y position - height
      positions_center[pos_index++] = -cur_j + imageHeight/2; // z position

      // left
      temp_i = max(cur_i-1,0); // replacing the left pos with the center pos if it is an edge
      height = heightmapImage->getPixel(temp_i, cur_j, 0);
      positions_left[pos_index_left++] = temp_i - imageWidth/2; // x position
      positions_left[pos_index_left++] = heightScale * height; // y position - height
      positions_left[pos_index_left++] = -cur_j + imageHeight/2; // z position

      // right
      temp_i = min(cur_i+1,imageWidth-1); // replacing the right pos with the center pos if it is an edge
      height = heightmapImage->getPixel(temp_i, cur_j, 0);
      positions_right[pos_index_right++] = temp_i - imageWidth/2; // x position
      positions_right[pos_index_right++] = heightScale * height; // y position - height
      positions_right[pos_index_right++] = -cur_j + imageHeight/2; // z position

      // down
      temp_j = max(cur_j-1,0); // replacing the down pos with the center pos if it is an edge
      height = heightmapImage->getPixel(cur_i, temp_j, 0);
      positions_down[pos_index_down++] = cur_i - imageWidth/2; // x position
      positions_down[pos_index_down++] = heightScale * height; // y position - height
      positions_down[pos_index_down++] = -temp_j + imageHeight/2; // z position

      // up
      temp_j = min(cur_j+1,imageHeight-1); // replacing the up pos with the center pos if it is an edge
      height = heightmapImage->getPixel(cur_i, temp_j, 0);
      positions_up[pos_index_up++] = cur_i - imageWidth/2; // x position
      positions_up[pos_index_up++] = heightScale * height; // y position - height
      positions_up[pos_index_up++] = -temp_j + imageHeight/2; // z position
    }
  }

  // --- Center & Color VBO ---
  // Create the VBOs. There is a single VBO in this example. This operation must be performed BEFORE we initialize any VAOs.
  glGenBuffers(1, &centerVBO);
  glBindBuffer(GL_ARRAY_BUFFER, centerVBO);
  // First, allocate an empty VBO of the correct size to hold positions and colors.
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions + numBytesInColors, nullptr, GL_STATIC_DRAW);
  // Next, write the position and color data into the VBO.
  glBufferSubData(GL_ARRAY_BUFFER, 0, numBytesInPositions, positions_center); // The VBO starts with positions.
  glBufferSubData(GL_ARRAY_BUFFER, numBytesInPositions, numBytesInColors, colors); // The colors are written after the positions.

  // --- Left VBO ---
  // Create the VBOs. There is a single VBO in this example. This operation must be performed BEFORE we initialize any VAOs.
  glGenBuffers(1, &leftVBO);
  glBindBuffer(GL_ARRAY_BUFFER, leftVBO);
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions, positions_left, GL_STATIC_DRAW);

  // --- Right VBO ---
  // Create the VBOs. There is a single VBO in this example. This operation must be performed BEFORE we initialize any VAOs.
  glGenBuffers(1, &rightVBO);
  glBindBuffer(GL_ARRAY_BUFFER, rightVBO);
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions, positions_right, GL_STATIC_DRAW);

  // --- Down VBO ---
  // Create the VBOs. There is a single VBO in this example. This operation must be performed BEFORE we initialize any VAOs.
  glGenBuffers(1, &downVBO);
  glBindBuffer(GL_ARRAY_BUFFER, downVBO);
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions, positions_down, GL_STATIC_DRAW);

  // --- Up VBO ---
  // Create the VBOs. There is a single VBO in this example. This operation must be performed BEFORE we initialize any VAOs.
  glGenBuffers(1, &upVBO);
  glBindBuffer(GL_ARRAY_BUFFER, upVBO);
  glBufferData(GL_ARRAY_BUFFER, numBytesInPositions, positions_up, GL_STATIC_DRAW);

  // --- VAO ---
  // Create the VAOs. There is a single VAO in this example.
  glGenVertexArrays(1, &smoothVAO);

  // --- Center ---
  glBindVertexArray(smoothVAO);
  glBindBuffer(GL_ARRAY_BUFFER, centerVBO); 

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint center_locationOfPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(center_locationOfPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(center_locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void *)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  // Set up the relationship between the "color" shader variable and the VAO.
  const GLuint smooth_locationOfColor = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color"); // Obtain a handle to the shader variable "color".
  glEnableVertexAttribArray(smooth_locationOfColor); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(smooth_locationOfColor, 4, GL_FLOAT, normalized, stride, (const void *)(unsigned long)numBytesInPositions); // The shader variable "color" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset "numBytesInPositions" in the VBO. There are 4 float entries per vertex in the VBO (i.e., r,g,b,a channels). 

  // --- Left ---
  glBindVertexArray(smoothVAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, leftVBO); 

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint left_locationOfPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position_left"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(left_locationOfPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(left_locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void *)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  // --- Right ---
  glBindVertexArray(smoothVAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rightVBO); 

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint right_locationOfPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position_right"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(right_locationOfPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(right_locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void *)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  // --- Down ---
  glBindVertexArray(smoothVAO);
  glBindBuffer(GL_ARRAY_BUFFER, downVBO); 

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint down_locationOfPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position_down"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(down_locationOfPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(down_locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void *)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 

  // --- Up ---
  glBindVertexArray(smoothVAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, upVBO); 

  // Set up the relationship between the "position" shader variable and the VAO.
  const GLuint up_locationOfPosition = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position_up"); // Obtain a handle to the shader variable "position".
  glEnableVertexAttribArray(up_locationOfPosition); // Must always enable the vertex attribute. By default, it is disabled.
  glVertexAttribPointer(up_locationOfPosition, 3, GL_FLOAT, normalized, stride, (const void *)0); // The shader variable "position" receives its data from the currently bound VBO (i.e., vertexPositionAndColorVBO), starting from offset 0 in the VBO. There are 3 float entries per vertex in the VBO (i.e., x,y,z coordinates). 


  // We don't need this data any more, as we have already uploaded it to the VBO. And so we can destroy it, to avoid a memory leak.
  free(positions_center);
  free(positions_left); 
  free(positions_right);
  free(positions_down);
  free(positions_up); 
  free(colors);


  // Check for any OpenGL errors.
  std::cout << "GL error: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
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

  // Perform the initialization.
  initScene(argc, argv);

  // Sink forever into the GLUT loop.
  glutMainLoop();
}


