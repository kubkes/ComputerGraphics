/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: Kübra Keskin
 * *************************
*/

#ifdef WIN32
  #include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
  #include <GL/gl.h>
  #include <GL/glut.h>
#elif defined(__APPLE__)
  #include <OpenGL/gl.h>
  #include <GLUT/glut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
  #define strcasecmp _stricmp
#endif

#include <imageIO.h>

#include <glm/glm.hpp>
// #include "openGLHeader.h"
// #include "glutHeader.h"
#include "math.h"
#include <iostream>
#include <cstring>
#include <vector>

#include <random>

using namespace std;

#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

char * filename = NULL;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

int mode = MODE_DISPLAY;

//you may want to make these smaller for debugging purposes
#define WIDTH 640
#define HEIGHT 480

//the field of view of the camera
#define fov 60.0

unsigned char buffer[HEIGHT][WIDTH][3];

struct Vertex
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double normal[3];
  double shininess;
};

struct Triangle
{
  Vertex v[3];
};

struct Sphere
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double shininess;
  double radius;
};

struct Light
{
  double position[3];
  double color[3];
};

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

// Random number generator (from course website)
std::random_device rd;
std::mt19937 eng;  // or eng(r()); for non-deterministic random number
std::uniform_real_distribution<double> distrib(0.0, 1.0 - 1e-8);

bool antialiasing = false; // control whether to apply antialiasing or not

int total_sampled_lights = 50; // number of sampled lights around the original light for soft shadow

int total_reflections = 0; // number of recursive reflections 

glm::vec3 cam_pos (0.0, 0.0, 0.0); // camera position

double epsilon = 0.0000001;

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel(int x,int y,unsigned char r,unsigned char g,unsigned char b);

glm::vec3 ray_trace(glm::vec3 r_p0, glm::vec3 r_d, double eps, glm::vec3& i_p, bool& intersected, int& shape, int& ind_min);

glm::vec3 shadow_ray(glm::vec3 r_origin, glm::vec3 i_p, glm::vec3 i_n, int shape, int ind);

glm::vec3 recursive_ray_trace(glm::vec3 r_origin, glm::vec3 r_dir, int reflection_count);

double intersect_with_sphere(glm::vec3 r_p0, glm::vec3 r_d, int s_ind);
glm::vec3 calculate_sphere_normal(glm::vec3 s_p, int s_ind);

double intersect_with_triangle(glm::vec3 r_p0, glm::vec3 r_d, int t_ind);
glm::vec3 calculate_triangle_normal(int t_ind);
bool check_intersection_inside_triangle(glm::vec3 i_p, glm::vec3 t_n, int t_ind);
double calculate_triangle_coefficient_d(int t_ind);
double calculate_2D_triangle_area(glm::vec3 a, glm::vec3 b, glm::vec3 c, int ind1, int ind2);

void calculate_barycentric_coordinates(int t_ind, glm::vec3 c, double& alpha, double& beta, double& gamma);

glm::vec3 calculate_phong_shading(Light l, glm::vec3 kd, glm::vec3 ks, double sh, glm::vec3 L, glm::vec3 N, glm::vec3 V, glm::vec3 R);

glm::vec3 normalize_vector(glm::vec3 v);
glm::vec3 cross_product(glm::vec3 u, glm::vec3 w);

glm::vec3 clamp_color_to_zero(glm::vec3 color);
glm::vec3 clamp_color_to_one(glm::vec3 color);
glm::vec3 clamp_color(glm::vec3 color);

glm::vec3 interpolate_three_double_vectors(double v1[], double v2[], double v3[], double alpha, double beta, double gamma);

glm::vec3 generate_random_point_inside_sphere();

glm::vec3 vec3_double3_add(glm::vec3 x, double *y);

double max3(double x, double y, double z);

//MODIFY THIS FUNCTION
void draw_scene()
{
  glm::vec3 color (0.0);

  // glm::vec3 color2 (0.0);
  glm::vec3 r_d (0.0);

  double a = (double)WIDTH/(double)HEIGHT; // aspect ratio
  double deg = tan(fov/2 * (double) M_PI /180.0);

  //a simple test output
  for(unsigned int x=0; x<WIDTH; x++)
  {
    glPointSize(2.0);  
    glBegin(GL_POINTS);
    for(unsigned int y=0; y<HEIGHT; y++)
    {
      glm::vec3 final_color (0.0);
      if(antialiasing)
      {
        for (int i = -1; i < 2; i=i+1) // i:{-1,0,1}
        {
          for (int j = -1; j < 2; j=j+1) // j:{-1,0,1}
          {
            r_d[0] = -a*deg + (2*(x+0.5*i)+1)*a*deg/(double)WIDTH;
            r_d[1] = -deg + (2*(y+0.5*j)+1)*deg/(double)HEIGHT;
            r_d[2] = -1.0;

            r_d = normalize_vector(r_d);

            color = recursive_ray_trace(cam_pos, r_d, 0);
            color = vec3_double3_add(color, ambient_light);
            color = clamp_color(color);

            final_color += color/9.0f;
          }
        }
      }
      else
      {
        r_d[0] = -a*deg + (2*x+1)*a*deg/(double)WIDTH;
        r_d[1] = -deg + (2*y+1)*deg/(double)HEIGHT;
        r_d[2] = -1.0;

        r_d = normalize_vector(r_d);

        color = recursive_ray_trace(cam_pos, r_d, 0);
        color = vec3_double3_add(color, ambient_light);
        final_color = clamp_color(color);
      }
      
      plot_pixel(x, y, final_color[0] * 255, final_color[1] * 255, final_color[2] * 255);
      // plot_pixel(x, y, x % 256, y % 256, (x+y) % 256);
    }
    glEnd();
    glFlush();
  }
  printf("Done!\n"); fflush(stdout);
}

// r_origin: ray origin
// r_dir: ray direction
// reflection_count: current number reflections
glm::vec3 recursive_ray_trace(glm::vec3 r_origin, glm::vec3 r_dir, int reflection_count)
{
  glm::vec3 phong_color (0.0);

  if(total_reflections == 0)
  {
    glm::vec3 i_p; // intersection point
    bool intersected = false; 
    int shape = 0; // shape (0: no shape, 1: sphere, 2: triangle)
    int ind; // index of the shape, if intersected
    // cout << "r_d: " << r_d[0] << ", " << r_d[1] << ", " << r_d[2] << endl;

    glm::vec3 i_n = ray_trace(r_origin, r_dir, (double) 0.0, i_p, intersected, shape, ind); // normal at the intersection point

    if(intersected)
    {
      phong_color = shadow_ray(r_origin, i_p, i_n, shape, ind);
    }
    else
    {
      phong_color = glm::vec3(1.0);
    }

    return phong_color;
  }
  else
  {
    if(reflection_count > total_reflections) // exit the recursion
    {
      return phong_color;
    }

    glm::vec3 i_p; // intersection point
    bool intersected = false; 
    int shape = 0; // shape (0: no shape, 1: sphere, 2: triangle)
    int ind; // index of the shape, if intersected
    glm::vec3 ks (0.0); // specular coefficient
    
    // Ray tracing
    glm::vec3 i_n = ray_trace(r_origin, r_dir, (double) 0.0001, i_p, intersected, shape, ind); // normal at the intersection point

    if(intersected)
    {
      // Calculate ks 
      if(shape == 1) 
      {
        ks[0] = spheres[ind].color_specular[0];
        ks[1] = spheres[ind].color_specular[1];
        ks[2] = spheres[ind].color_specular[2];
      }
      if(shape == 2) 
      {
        double alpha;
        double beta;
        double gamma;

        calculate_barycentric_coordinates(ind, i_p, alpha, beta, gamma);
        ks = interpolate_three_double_vectors(triangles[ind].v[0].color_specular, 
          triangles[ind].v[1].color_specular, triangles[ind].v[2].color_specular, alpha, beta, gamma);
      }
    }

    if(intersected)
    {
      phong_color = shadow_ray(r_origin, i_p, i_n, shape, ind);
      phong_color = clamp_color(phong_color);
    }
    else
    {
      phong_color = glm::vec3(1.0);
    }

    glm::vec3 ref_dir = normalize_vector(-2.0f*(glm::dot(r_dir,i_n))*i_n + r_dir); // reflection ray direction

    float ref_rate = 0.1f; // reflection rate

    glm::vec3 ref_color = recursive_ray_trace(i_p, ref_dir, reflection_count+1); // color from reflection

    glm::vec3 final_color;

    if(reflection_count == 0)
    {
      final_color = ref_rate*ref_color + (1-ref_rate)*phong_color;
    }
    else
    {
      final_color[0] = ks[0]*ref_color[0] + (1-ks[0])*phong_color[0];
      final_color[1] = ks[1]*ref_color[1] + (1-ks[1])*phong_color[1];
      final_color[2] = ks[2]*ref_color[2] + (1-ks[2])*phong_color[2];

    }
    return (1-ref_rate)*final_color + ref_rate*recursive_ray_trace(i_p, ref_dir, reflection_count+2);
  }
}


// r_origin: ray origin
// i_p: intersection point
// i_n: normal at the intersection point
// shape: (0: no shape, 1: sphere, 2: triangle)
// ind: index of the shape, if intersected
glm::vec3 shadow_ray(glm::vec3 r_origin, glm::vec3 i_p, glm::vec3 i_n, int shape, int ind)
{
  int s_shape = 0; // shadow hit shape
  int s_ind = -1; // shadow hit index
  glm::vec3 color (0.0);
  for (int i = 0; i < num_lights; i++)
  {
    glm::vec3 light_color (0.0);
    for (int j = 0; j < total_sampled_lights; j++)
    {
      glm::vec3 l_p (0.0); // light's position
      glm::vec3 r_d (0.0); // ray direction
      glm::vec3 s_p (0.0); // shadow ray intersection point
      bool isShadow = false;
      glm::vec3 local_color;
      glm::vec3 L; // light vector
      glm::vec3 N; // normal vector
      glm::vec3 R; // reflected vector
      glm::vec3 V; // view vector

      glm::vec3 kd; // diffuse coefficient
      glm::vec3 ks; // specular coefficient
      double sh; // shineness

      if(j == 0)
      {
        l_p[0] = lights[i].position[0];
        l_p[1] = lights[i].position[1];
        l_p[2] = lights[i].position[2];
      }
      else
      {
        glm::vec3 rand_point = 0.2f*generate_random_point_inside_sphere();
        l_p[0] = lights[i].position[0] + rand_point[0];
        l_p[1] = lights[i].position[1] + rand_point[1];
        l_p[2] = lights[i].position[2] + rand_point[2];
      }

      r_d = normalize_vector(l_p-i_p);

      ray_trace(i_p, r_d, (double) 0.0005, s_p, isShadow, s_shape, s_ind);

      if(s_shape == shape && s_ind == ind && isShadow)
      {
        isShadow = false;
      }
      if(glm::length(i_p-s_p) < epsilon && isShadow)
      {
        isShadow = false;
      }
      if(glm::length(s_p-i_p) > glm::length(l_p-i_p))
      {
        isShadow = false;
      }

      if(!isShadow)
      {
        if(shape == 1) // Phong Shading for sphere
        {
          kd[0] = spheres[ind].color_diffuse[0];
          kd[1] = spheres[ind].color_diffuse[1];
          kd[2] = spheres[ind].color_diffuse[2];

          ks[0] = spheres[ind].color_specular[0];
          ks[1] = spheres[ind].color_specular[1];
          ks[2] = spheres[ind].color_specular[2];

          sh = spheres[ind].shininess;

          L = r_d; // light direction
          N = i_n; // normal direction
          R = normalize_vector(2.0f*(glm::dot(L,N))*N - L); // reflection direction
          V = normalize_vector(r_origin-i_p); // view direction

          local_color = calculate_phong_shading(lights[i], kd, ks, sh, L, N, V, R);
        }
        else if(shape == 2) // Phong shading for triangle
        {
          double alpha;
          double beta;
          double gamma;

          calculate_barycentric_coordinates(ind, i_p, alpha, beta, gamma);

          kd = interpolate_three_double_vectors(triangles[ind].v[0].color_diffuse, 
            triangles[ind].v[1].color_diffuse, triangles[ind].v[2].color_diffuse, alpha, beta, gamma);

          ks = interpolate_three_double_vectors(triangles[ind].v[0].color_specular, 
            triangles[ind].v[1].color_specular, triangles[ind].v[2].color_specular, alpha, beta, gamma);

          sh = alpha*triangles[ind].v[0].shininess + beta*triangles[ind].v[1].shininess 
            + gamma*triangles[ind].v[2].shininess;

          L = r_d; // light direction
          N = normalize_vector(interpolate_three_double_vectors(triangles[ind].v[0].normal, 
            triangles[ind].v[1].normal, triangles[ind].v[2].normal, alpha, beta, gamma)); // normal direction
          R = normalize_vector(2.0f*(glm::dot(L,N))*N - L); // reflection direction
          V = normalize_vector(r_origin-i_p); // view direction

          local_color = calculate_phong_shading(lights[i], kd, ks, sh, L, N, V, R);
          // local_color = calculate_triangle_normal(ind);
        }
        light_color = light_color + local_color;
      }
    }
    color = color + light_color/(float)total_sampled_lights;
  }
  return color;
}

// reference: http://datagenetics.com/blog/january32020/index.html
glm::vec3 generate_random_point_inside_sphere()
{
  glm::vec3 rand_point;
  double theta = distrib(eng) * 2 * M_PI; // random number between 0 and 2pi
  double v = distrib(eng); // random number between 0 and 1
  double phi = acos(2*v-1); 
  double r = pow(distrib(eng),1/3); 
  rand_point[0] = r * sin(phi) * cos(theta);
  rand_point[1] = r * sin(phi) * sin(theta);
  rand_point[0] = r * cos(phi);

  return rand_point;
}


glm::vec3 interpolate_three_double_vectors(double v1[], double v2[], double v3[], double alpha, double beta, double gamma)
{
  glm::vec3 n1(v1[0], v1[1], v1[2]);
  glm::vec3 n2(v2[0], v2[1], v2[2]);
  glm::vec3 n3(v3[0], v3[1], v3[2]);

  // interpolated vector
  glm::vec3 n = (float)alpha * n1 + (float)beta * n2 + (float)gamma * n3; 

  return n;
}


void calculate_barycentric_coordinates(int t_ind, glm::vec3 c, double& alpha, double& beta, double& gamma)
{
  glm::vec3 v1(0.0);
  glm::vec3 v2(0.0);
  glm::vec3 v3(0.0);

  v1[0] = triangles[t_ind].v[0].position[0];
  v1[1] = triangles[t_ind].v[0].position[1];
  v1[2] = triangles[t_ind].v[0].position[2];

  v2[0] = triangles[t_ind].v[1].position[0];
  v2[1] = triangles[t_ind].v[1].position[1];
  v2[2] = triangles[t_ind].v[1].position[2];

  v3[0] = triangles[t_ind].v[2].position[0];
  v3[1] = triangles[t_ind].v[2].position[1];
  v3[2] = triangles[t_ind].v[2].position[2];

  double a_v1v2v3 = glm::length(cross_product(v2-v1,v3-v1));
  double a_cv2v3 = glm::length(cross_product(v2-c,v3-c));
  double a_v1cv3 = glm::length(cross_product(c-v1,v3-v1));

  alpha = a_cv2v3/a_v1v2v3;
  beta = a_v1cv3/a_v1v2v3;
  gamma = 1-alpha-beta;
}

glm::vec3 cross_product(glm::vec3 u, glm::vec3 w)
{
  glm::vec3 n(0.0);

  // cross product of u and w
  n[0] = u[1]*w[2] - u[2]*w[1];
  n[1] = u[2]*w[0] - u[0]*w[2];
  n[2] = u[0]*w[1] - u[1]*w[0];
  
  return n;
}

glm::vec3 calculate_phong_shading(Light l, glm::vec3 kd, glm::vec3 ks, double sh, glm::vec3 L, glm::vec3 N, glm::vec3 V, glm::vec3 R)
{
  glm::vec3 I(0.0); // local phong shading color

  double LdotN = glm::dot(L,N);
  if(LdotN < 0.0)
  {
    LdotN = 0.0;
  }

  double RdotV = glm::dot(R,V);
  if(RdotV < 0.0)
  {
    RdotV = 0.0;
  }

  I[0] = l.color[0] * (kd[0]*LdotN + ks[0]*pow(RdotV,sh));
  I[1] = l.color[1] * (kd[1]*LdotN + ks[1]*pow(RdotV,sh));
  I[2] = l.color[2] * (kd[2]*LdotN + ks[2]*pow(RdotV,sh));

  return I;
}


// r_p0: ray origin
// r_d: ray direction vector (normalized)
// eps: small value to be used in comparisons
// i_p: intersection point (output)
// intersected: boolean to inform if an intersection found (output)
// shape: to inform if the ray intersected with a sphere or triangle (output)
// ind: indices of the shape where ray intersected (output)
// output: normal at the intersection point
glm::vec3 ray_trace(glm::vec3 r_p0, glm::vec3 r_d, double eps, glm::vec3& i_p, bool& intersected, int& shape, int& ind_min)
{
  double t;
  double t_min_sphere = INFINITY; // minimum t value for sphere
  double t_min_triangle = INFINITY; // minimum t value for triangle
  int ind_min_sphere = 0; // index of the minimum t value for sphere
  int ind_min_triangle = 0; // index of the minimum t value for sphere
  // glm::vec3 i_p (0.0); // intersection point
  glm::vec3 color (0.0);

  for (int i = 0; i < num_spheres; i++) // loop over all spheres
  {
    t = intersect_with_sphere(r_p0, r_d, i);
    if(t > eps)
    {
      if(t < t_min_sphere)
      {
        t_min_sphere = t;
        ind_min_sphere = i;
      }
    }
  }

  for (int i = 0; i < num_triangles; i++) // loop over all triangles
  {
    t = intersect_with_triangle(r_p0, r_d, i);
    if(t > eps)
    {
      if(t < t_min_triangle)
      {
        t_min_triangle = t;
        ind_min_triangle = i;
      }
    }
  }

  if(t_min_sphere < t_min_triangle)
  {
    shape = 1; //sphere
    intersected = true;

    ind_min = ind_min_sphere;

    i_p[0] = r_p0[0]+r_d[0]*t_min_sphere;
    i_p[1] = r_p0[1]+r_d[1]*t_min_sphere;
    i_p[2] = r_p0[2]+r_d[2]*t_min_sphere;

    color = calculate_sphere_normal(i_p, ind_min);
  }
  else if(t_min_triangle < t_min_sphere)
  {
    shape = 2; // triangle
    intersected = true;

    ind_min = ind_min_triangle;

    i_p[0] = r_p0[0]+r_d[0]*t_min_triangle;
    i_p[1] = r_p0[1]+r_d[1]*t_min_triangle;
    i_p[2] = r_p0[2]+r_d[2]*t_min_triangle;

    color = calculate_triangle_normal(ind_min);
  }
  return color;
}

// s_p: point on the sphere
// s_c: center of the sphere
// s_r: radius of the sphere
glm::vec3 calculate_sphere_normal(glm::vec3 s_p, int s_ind)
{
  glm::vec3 s_c (0.0);
  s_c[0] = spheres[s_ind].position[0];
  s_c[1] = spheres[s_ind].position[1];
  s_c[2] = spheres[s_ind].position[2];

  double s_r = spheres[s_ind].radius;

  return normalize_vector(glm::vec3((s_p[0]-s_c[0])/s_r, (s_p[1]-s_c[1])/s_r, (s_p[2]-s_c[2])/s_r));
}

// normalize vector function to handle nan values
glm::vec3 normalize_vector(glm::vec3 v)
{
  // if the vector is all zeros, return the zero vector, not normalize to avoid nan values
  if(fabs(glm::length(v)) == 0.0)
  {
    return v;
  }
  else
  {
    return glm::normalize(v);
  }
}

// r_p0: ray origin
// r_d: ray direction vector (normalized)
// s_ind: sphere index
// s_c: sphere center
// s_r: sphere radius
double intersect_with_sphere(glm::vec3 r_p0, glm::vec3 r_d, int s_ind)
{
  glm::vec3 s_c (0.0);
  s_c[0] = spheres[s_ind].position[0];
  s_c[1] = spheres[s_ind].position[1];
  s_c[2] = spheres[s_ind].position[2];

  double s_r = spheres[s_ind].radius;
  
  double b;
  double c;

  double t;

  b = 2*(r_d[0]*(r_p0[0]-s_c[0]) + r_d[1]*(r_p0[1]-s_c[1]) + r_d[2]*(r_p0[2]-s_c[2]));  
  c = pow(r_p0[0]-s_c[0],2) + pow(r_p0[1]-s_c[1],2) + pow(r_p0[2]-s_c[2],2) - pow(s_r,2);

  double det = pow(b,2) - 4*c; // determinant

  if(det < 0.0) // no root
  {
    t = -1.0; // set to -1 to inform that there is no intersection
  }
  else if(fabs(det) < epsilon) // double root
  {
    if(b >= 0.0)
    {
      t = -1.0; // no intersection
    }
    else
    {
      t = -b/2;
    }
  }
  else // two different roots
  {
    double t0 = (- b - sqrt(det))*0.5;
    double t1 = (- b + sqrt(det))*0.5;

    // cout << "t0: " << t0 << ", t1: " << t1 << endl;
    if(t0 > 0.0)
    {
      t = t0;
    }
    else if(t1 > 0.0)
    {
      t = t1;
    }
    else
    {
      t = -1.0; // no intersection
    }
  }
  return t;
}

// r_p0: ray origin
// r_d: ray direction vector (normalized)
// t_ind: triangle index
double intersect_with_triangle(glm::vec3 r_p0, glm::vec3 r_d, int t_ind)
{
  glm::vec3 t_n = calculate_triangle_normal(t_ind);

  glm::vec3 i_p(0.0); // intersection point

  double d = calculate_triangle_coefficient_d(t_ind);

  double n_dot_d = glm::dot(t_n,r_d);

  double t;

  if(fabs(n_dot_d) < epsilon)
  {
    t = -1.0; // no intersection
  }
  else
  {
    t = -(glm::dot(t_n,r_p0)+d)/n_dot_d;
  }

  if(t > 0.0)
  {
    i_p[0] = r_p0[0]+r_d[0]*t;
    i_p[1] = r_p0[1]+r_d[1]*t;
    i_p[2] = r_p0[2]+r_d[2]*t;

    // check if the intersection is inside the triangle
    if(!check_intersection_inside_triangle(i_p, t_n, t_ind))
    {
      t = -1.0; // no intersection
    }
  }
  return t;
}

// i_p: intersection point
// t_n: normal of the triangle
// t_ind: triangle index
bool check_intersection_inside_triangle(glm::vec3 i_p, glm::vec3 t_n, int t_ind)
{
  // triangle vertices a, b, and c counter-clockwise
  glm::vec3 a(0.0);
  glm::vec3 b(0.0);
  glm::vec3 c(0.0);

  a[0] = triangles[t_ind].v[0].position[0];
  a[1] = triangles[t_ind].v[0].position[1];
  a[2] = triangles[t_ind].v[0].position[2];

  b[0] = triangles[t_ind].v[1].position[0];
  b[1] = triangles[t_ind].v[1].position[1];
  b[2] = triangles[t_ind].v[1].position[2];

  c[0] = triangles[t_ind].v[2].position[0];
  c[1] = triangles[t_ind].v[2].position[1];
  c[2] = triangles[t_ind].v[2].position[2];

  // dot product between normal of the triangle and the normal of the 2D plane
  double proj_xy = fabs(glm::dot(t_n, glm::vec3(0.0,0.0,1.0)));
  double proj_yz = fabs(glm::dot(t_n, glm::vec3(1.0,0.0,0.0)));
  double proj_xz = fabs(glm::dot(t_n, glm::vec3(0.0,1.0,0.0)));

  // the one with the maximum dot product will be the projection plane
  double max_proj = max3(proj_xy, proj_yz, proj_xz);

  int ind1;
  int ind2;

  if(fabs(proj_xy-max_proj) < epsilon) // Project onto the xy plane
  {
    ind1 = 0; ind2 = 1;
  }
  else if(fabs(proj_yz-max_proj) < epsilon) // Project onto the yz plane
  {
    ind1 = 1; ind2 = 2;
  }
  else // Project onto the xz plane
  {
    ind1 = 0; ind2 = 2;
  }

  double t_area = calculate_2D_triangle_area(a, b, c, ind1, ind2);
  double alpha = calculate_2D_triangle_area(i_p, b, c, ind1, ind2)/t_area;
  double beta = calculate_2D_triangle_area(a, i_p, c, ind1, ind2)/t_area;
  double gamma = 1-alpha-beta;

  if(alpha >= 0.0 && beta >= 0.0 && gamma >= 0.0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

// ind1: first index
// ind2: second index
// xy-projection: ind1 = 0, ind2 = 1
// yz-projection: ind1 = 1, ind2 = 2
// xz-projection: ind1 = 0, ind2 = 2
double calculate_2D_triangle_area(glm::vec3 a, glm::vec3 b, glm::vec3 c, int ind1, int ind2)
{
  return 0.5 * ((b[ind1]-a[ind1])*(c[ind2]-a[ind2])-(c[ind1]-a[ind1])*(b[ind2]-a[ind2]));
}

// calculate the normal of the triangle whose vertices are v1, v2, and v3 counter-clockwise
// t_ind: index of the triangle
glm::vec3 calculate_triangle_normal(int t_ind)
{
  glm::vec3 v1(0.0);
  glm::vec3 v2(0.0);
  glm::vec3 v3(0.0);

  v1[0] = triangles[t_ind].v[0].position[0];
  v1[1] = triangles[t_ind].v[0].position[1];
  v1[2] = triangles[t_ind].v[0].position[2];

  v2[0] = triangles[t_ind].v[1].position[0];
  v2[1] = triangles[t_ind].v[1].position[1];
  v2[2] = triangles[t_ind].v[1].position[2];

  v3[0] = triangles[t_ind].v[2].position[0];
  v3[1] = triangles[t_ind].v[2].position[1];
  v3[2] = triangles[t_ind].v[2].position[2];

  glm::vec3 u = v2-v1;
  glm::vec3 w = v3-v1;
  glm::vec3 n(0.0f);

  // cross product of u and w
  n = cross_product(u,w);

  return normalize_vector(n);
}

// calculate the coefficient d of the triangle where ax+by+cz+d=0 and n=[a b c]
// t_ind: index of the triangle
double calculate_triangle_coefficient_d(int t_ind)
{
  glm::vec3 t_n = calculate_triangle_normal(t_ind);

  glm::vec3 v1(0.0);

  v1[0] = triangles[t_ind].v[0].position[0];
  v1[1] = triangles[t_ind].v[0].position[1];
  v1[2] = triangles[t_ind].v[0].position[2];

  return -(double)glm::dot(v1,t_n);
}

glm::vec3 clamp_color_to_zero(glm::vec3 color)
{
  if(color[0] < 0.0)
  {
    color[0] = 0.0;
  }

  if(color[1] < 0.0)
  {
    color[1] = 0.0;
  }

  if(color[2] < 0.0)
  {
    color[2] = 0.0;
  }
  
  return color;
}

glm::vec3 clamp_color_to_one(glm::vec3 color)
{
  if(color[0] > 1.0)
  {
    color[0] = 1.0;
  }

  if(color[1] > 1.0)
  {
    color[1] = 1.0;
  }

  if(color[2] > 1.0)
  {
    color[2] = 1.0;
  }
  
  return color;
}

glm::vec3 clamp_color(glm::vec3 color)
{
  return clamp_color_to_one(clamp_color_to_zero(color));
}

glm::vec3 vec3_double3_add(glm::vec3 x, double *y)
{
  glm::vec3 z(0.0);
  z[0] = x[0] + y[0];
  z[1] = x[1] + y[1];
  z[2] = x[2] + y[2];
  return z;
}

double max3(double x, double y, double z)
{
  if(x > max(y,z))
  {
    return x;
  }
  else if(y > max(x,z))
  {
    return y;
  }
  else if(z > max(x,y))
  {
    return z;
  }
  else
  {
    return x;
  }

}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
  glVertex2i(x,y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  buffer[y][x][0] = r;
  buffer[y][x][1] = g;
  buffer[y][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  if(mode == MODE_JPEG)
    plot_pixel_jpeg(x,y,r,g,b);
}

void save_jpg()
{
  printf("Saving JPEG file: %s\n", filename);

  ImageIO img(WIDTH, HEIGHT, 3, &buffer[0][0][0]);
  if (img.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
    printf("Error in Saving\n");
  else 
    printf("File saved Successfully\n");
}

void parse_check(const char *expected, char *found)
{
  if(strcasecmp(expected,found))
  {
    printf("Expected '%s ' found '%s '\n", expected, found);
    printf("Parse error, abnormal abortion\n");
    exit(0);
  }
}

void parse_doubles(FILE* file, const char *check, double p[3])
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
  printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}

void parse_rad(FILE *file, double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  printf("rad: %f\n",*r);
}

void parse_shi(FILE *file, double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
  FILE * file = fopen(argv,"r");
  int number_of_objects;
  char type[50];
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file,"%i", &number_of_objects);

  printf("number of objects: %i\n",number_of_objects);

  parse_doubles(file,"amb:",ambient_light);

  for(int i=0; i<number_of_objects; i++)
  {
    fscanf(file,"%s\n",type);
    printf("%s\n",type);
    if(strcasecmp(type,"triangle")==0)
    {
      printf("found triangle\n");
      for(int j=0;j < 3;j++)
      {
        parse_doubles(file,"pos:",t.v[j].position);
        parse_doubles(file,"nor:",t.v[j].normal);
        parse_doubles(file,"dif:",t.v[j].color_diffuse);
        parse_doubles(file,"spe:",t.v[j].color_specular);
        parse_shi(file,&t.v[j].shininess);
      }

      if(num_triangles == MAX_TRIANGLES)
      {
        printf("too many triangles, you should increase MAX_TRIANGLES!\n");
        exit(0);
      }
      triangles[num_triangles++] = t;
    }
    else if(strcasecmp(type,"sphere")==0)
    {
      printf("found sphere\n");

      parse_doubles(file,"pos:",s.position);
      parse_rad(file,&s.radius);
      parse_doubles(file,"dif:",s.color_diffuse);
      parse_doubles(file,"spe:",s.color_specular);
      parse_shi(file,&s.shininess);

      if(num_spheres == MAX_SPHERES)
      {
        printf("too many spheres, you should increase MAX_SPHERES!\n");
        exit(0);
      }
      spheres[num_spheres++] = s;
    }
    else if(strcasecmp(type,"light")==0)
    {
      printf("found light\n");
      parse_doubles(file,"pos:",l.position);
      parse_doubles(file,"col:",l.color);

      if(num_lights == MAX_LIGHTS)
      {
        printf("too many lights, you should increase MAX_LIGHTS!\n");
        exit(0);
      }
      lights[num_lights++] = l;
    }
    else
    {
      printf("unknown type in scene description:\n%s\n",type);
      exit(0);
    }
  }
  return 0;
}

void display()
{
}

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
  //hack to make it only draw once
  static int once=0;
  if(!once)
  {
    draw_scene();
    if(mode == MODE_JPEG)
      save_jpg();
  }
  once=1;
}

int main(int argc, char ** argv)
{
  string true_str ("true");
  if ((argc < 2) || (argc > 6))
  {  
    printf ("Usage: %s <input scenefile> <int #reflections> <bool antialiasing> <int #softshadowlights> [output jpegname]\n", argv[0]);
    exit(0);
  }
  if(argc == 2)
  {
    total_reflections = 0;
    antialiasing = false;
    total_sampled_lights = 1;
    mode = MODE_DISPLAY;
  }
  else if(argc == 3)
  {
    total_reflections = stoi(argv[2]);

    antialiasing = false;
    total_sampled_lights = 1;
    mode = MODE_DISPLAY;
  }
  else if(argc == 4)
  {
    total_reflections = stoi(argv[2]);
    antialiasing = (true_str.compare(argv[3]) == 0);

    total_sampled_lights = 1;
    mode = MODE_DISPLAY;
  }
  else if(argc == 5)
  {
    total_reflections = stoi(argv[2]);
    antialiasing = (true_str.compare(argv[3]) == 0);
    total_sampled_lights = stoi(argv[4]);
    mode = MODE_DISPLAY;    
  }
  else if(argc == 6)
  {
    total_reflections = stoi(argv[2]);
    antialiasing = (true_str.compare(argv[3]) == 0);
    total_sampled_lights = stoi(argv[4]);  

    mode = MODE_JPEG;
    filename = argv[5];  
  }

  glutInit(&argc,argv);
  loadScene(argv[1]);
  
  printf ("\nNumber of reflections: %i \n", total_reflections);
  if(antialiasing) printf ("Antialiasing: applied\n");
  if(!antialiasing) printf ("Antialiasing: none\n");
  printf ("Number of soft shadow lights: %i \n", total_sampled_lights);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(WIDTH - 1, HEIGHT - 1);
  #endif
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
}

