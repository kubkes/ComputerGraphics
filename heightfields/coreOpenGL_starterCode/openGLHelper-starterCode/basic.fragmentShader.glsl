#version 150

in vec4 col;
out vec4 c;

uniform int colorChangeEnable;
uniform float mouseX;
uniform float mouseY;

void main()
{
  // compute the final pixel color

  // if color change is enabled, render will be colored
  if( colorChangeEnable == 1 )
  {
    c[0] = mouseX;
    c[1] = col[1];
    c[2] = mouseY;
    c[3] = 1.0;   
  }
  else // otherwise it will be grayscale
  {
    c = col;
  }
}

