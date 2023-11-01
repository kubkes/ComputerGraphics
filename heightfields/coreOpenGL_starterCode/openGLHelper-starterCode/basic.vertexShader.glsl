#version 150

in vec3 position;
in vec4 color;
out vec4 col;

in vec3 position_left;
in vec3 position_right;
in vec3 position_up;
in vec3 position_down;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

uniform int mode;

uniform float maxHeight;


void main()
{
  // compute the transformed and projected vertex position (into gl_Position) 
  // compute the vertex color (into col)

  // if mode is 1 smooth the terrain with the help of neigboring vertices
  if (mode == 1)
  {
    gl_Position = projectionMatrix * modelViewMatrix * vec4((position_left + position_right + position_down + position_up)/4, 1.0f);
    float smoothenedHeight = (position_left.y + position_right.y + position_down.y + position_up.y)/4;
    
    col[0] = smoothenedHeight / maxHeight;
    col[1] = smoothenedHeight / maxHeight;
    col[2] = smoothenedHeight / maxHeight;
    col[3] = 1.0;

  }
  else
  {
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
    
    col = color;
  }
}

