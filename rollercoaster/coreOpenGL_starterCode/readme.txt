Subject 	: CSCI420 - Computer Graphics 
Assignment 2	: Simulating a Roller Coaster

Description: In this assignment, we use Catmull-Rom splines along with OpenGL core profile shader-based texture mapping and Phong shading to create a roller coaster simulation.

Core Credit Features: 
======================

1. Uses OpenGL core profile, version 3.2 or higher 

2. Completed all Levels:
  Level 1 : - Yes
  level 2 : - Yes
  Level 3 : - Yes
  Level 4 : - Yes
  Level 5 : - Yes

3. Rendered the camera at a reasonable speed in a continuous path/orientation 

4. Run at interactive frame rate (>15fps at 1280 x 720) - Yes (I set it to 30 fps, but it can be set to higher rates too)

5. Understandably written, well commented code

Extra Credit Features:
======================

1. Render a T-shaped rail cross section 

    Rail has an inverted T-shaped cross-section, widths and heights of the T-shape's top and bottom parts can be customizable.
    ``alphaN`` and ``alphaB`` controls the height and width of the top of the rail, respectively
    ``betaN`` and ``betaB`` controls the height and width of the bottom of the rail, respectively

2. Render a Double Rail 

    ``tubeHalfDist`` controls the distance between double rails

3. Any Additional Scene Elements? (list them here) 

    I added sleepers to the double rails to make it look more realistic.
    Gaps between sleepers are created by skipping some amount of points.
    ``sleeperSkip`` controls how much gap will be created between sleepers.

4. Render a sky-box

    I used a sky-box cube to render a more realistic ground, sky, and environment in the scene.
    For this reason, I used an HDRI image from this link: "https://polyhaven.com/a/lilienstein".
    This HDRI image is converted into 6 images for the 6 faces of the cubes by using this link: 
    "https://matheowis.github.io/HDRI-to-CubeMap/"
    I calculated the positions and UV locations of each face (negative x, y, z and positive x, y, z)

5. Generate track from several sequences of splines

    The code can handle as many splines as possible by adding them back to back.
    I added the last position of the last spline as an offset to the new spline to make this possible.
    (Note: Tracks might intersect at some locations, since there is no control over 
    whether some tracks will be intersected at some locations of the rails)

6. Render environment in a better manner - Yes (a little)

    I used the following online tool to pick phong shader parameters to reflect the metallic brown color of the tracks as much as possible:
    "http://www.cs.toronto.edu/~jacobson/phong-demo/"

7. Modify velocity with which the camera moves

    There are two modes for the camera speed in the program. At default, camera moves with a speed varying in u
    according to the equation given in the homework extras: u_new = u_old + delta_t * sqrt(2*g*(h_max-h))/ mag(dp/du)
    In the other mode, camera moves with a constant speed in u as given in the homework description.
    The user can switch between the constant speed mode or the varying speed mode by pressing 
    'u' (constant) or 'y' (varying) while the animation running.


Additional Features: 
1.  Animation does not start initially when the program is first opened. When user presses the key 'r', animation starts.
    If the user would like to explore the environment, they can press 't' to stop the animation, 
    and look around by mouse and keyboard controls. When 'r' is pressed, the animation scene is starting where it left before.

2.  There is an additional pipeline program to render the spline as lines, this can be seen when the key '2' is pressed.
    The extra feature of this shader is that the color of the spline can be changed based on the mouse position. 
    This is enabled when the key '5' is pressed, and it makes the current rendered line colored.
	  If key 'f' is pressed, the color change will be frozen, and the line will have the latest color.
	  If key 'c' is pressed, the color change will be enabled again.

3.  The user can switch between the line rendering mode or the rendering of the rail tracks by pressing '2' (lines) or '3' (tracks) while the animation running, the animation continues without stopping.


Keyboard/Mouse controls: (Please document Keyboard/Mouse controls if any)
1.  'w': Translate
2.  's': Starts automatic screenshots up to 1000 frames
3.  'r': Starts/continues the automatic ride
4.  't': Stops the automatic ride
5.  'u': camera speed constant in u
6.  'y': camera speed varying in u
7.  '2': Line rendering
8.  '3': Track rendering
9.  '5': Color change mode for the line rendering mode
10. 'c': Color change starts
11. 'f': Color change freezes


