﻿#include "linePipelineProgram.h"
#include "openGLHeader.h"
#include <iostream>
#include <cstring>

using namespace std;

int LinePipelineProgram::Init(const char * shaderBasePath) 
{
  if (BuildShadersFromFiles(shaderBasePath, "line.vertexShader.glsl", "line.fragmentShader.glsl") != 0)
  {
    cout << "Failed to build the pipeline program." << endl;
    return 1;
  }

  cout << "Successfully built the pipeline program." << endl;
  return 0;
}

void LinePipelineProgram::SetModelViewMatrix(const float * m) 
{
  // Pass "m" to the pipeline program, as the modelview matrix.
  glUniformMatrix4fv(h_modelViewMatrix, 1, GL_FALSE, m);
}

void LinePipelineProgram::SetProjectionMatrix(const float * m) 
{
  // Pass "m" to the pipeline program, as the projection matrix.
  glUniformMatrix4fv(h_projectionMatrix, 1, GL_FALSE, m);
}

int LinePipelineProgram::SetShaderVariableHandles() 
{
  // Set h_modelViewMatrix and h_projectionMatrix
  SET_SHADER_VARIABLE_HANDLE(modelViewMatrix);
  SET_SHADER_VARIABLE_HANDLE(projectionMatrix);
  return 0;
}

