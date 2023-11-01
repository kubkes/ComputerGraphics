Assignment #3: Ray tracing

FULL NAME: Kübra Keskin


MANDATORY FEATURES
------------------

Feature:                                 Status: finish? (yes/no)
-------------------------------------    -------------------------
1) Ray tracing triangles                  yes

2) Ray tracing sphere                     yes

3) Triangle Phong Shading                 yes

4) Sphere Phong Shading                   yes

5) Shadows rays                           yes

6) Still images                           yes
   
7) Extra Credit (up to 30 points)
   - Recursive Reflection:
      I implemented recursive reflection by recursively calling the ray tracing function from the point it hit previously.
      Number of reflections is set by command line input.
      Still images including reflection results for three sample scenes: 
            "spheres_reflect1.jpg" 
            "SIGGRAPH_reflect1.jpg"
            "test2_reflect1.jpg"

   - Antialiasing:
      I implemented antialiasing by supersampling a pixel by 9 times (rays sent from a 3x3 grid).
      (How much we supersample can be changed inside the code.)
      Whether to apply antialiasing or not is set by command line input.
      Still images including antialiasing results for two sample scenes: 
            "test1_antialising.jpg" 
            "test2_antialising.jpg"

   - Soft Shadows:
      I implemented soft shadows by applying multiple numbers of lights per each light in the scene. This new light location is chosen randomly from a sphere where the center of the sphere is the original light.
      Number of soft shadow lights applied is set by command line input.
      Still images include soft shadow results for two sample scenes: 
            "table_soft100.jpg"
            "snow_soft20.jpg"

      Some others:
            "spheres_reflect1_antialiasing.jpg"
            "test2_reflect1_antialiasing_soft100.jpg"

   Note: I only provided a couple of outputs for each feature and with some features applied together.
         However any of the extra features can be run with the other extras, but it might take some time to render in some scenes.
         How to setup parameters for these features are explained below.
   
Command line usage:

"./hw3 <input scenefile> <int #reflections> <bool antialiasing> <int #softshadowlights> [output jpegname]"

If the arguments after the input scenefile are not provided, they are set to default values for the ones that are not provided
(Default: no reflection, no antialiasing, no soft shadow).

Note: 
still_images_name folder includes all the images mentioned above.

still_images_number folder includes only the selected ones.