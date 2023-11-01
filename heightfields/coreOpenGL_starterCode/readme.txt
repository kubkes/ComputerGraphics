Subject 	: CSCI420 - Computer Graphics 
Assignment 1	: Height Fields Using Shaders

Functionalities of the program:

- Translation is done with the key 'W' pressed, and middle (z-translation) and left (x,y-translation) mouse buttons being dragged
- Scale is done with the key 'Shift' pressed, and middle (z-scale) and left (x,y-scale) mouse buttons being dragged
- Rotation is done when no key is pressed, and middle (z-rotation) and left (x,y-rotation) mouse buttons being dragged

Other key functionalities:
- '1': Points
- '2': Lines
- '3': Triangles
- '4': Smoothing
       (Smoothing is done by passing the maximum height of the terrain to the vertex shader)
- '5': Enable color change
	This key can be pressed after any of the '1,2,3,4' keys, it makes the current rendered scene colored
	Color is changed based on the mouse position
	if key 'f' is pressed, the color change will be frozen, and the scene will have the latest color
	if key 'c' is pressed, the color change will remain
- 's': automatic screenshot until 300 frames are captured (15 fps)
- 'x': take one screenshot
