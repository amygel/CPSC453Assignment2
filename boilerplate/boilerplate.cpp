// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <iterator>
#include <vector>

#define _USE_MATH_DEFINES
#include <math.h>

// specify that we want the OpenGL core profile before including GLFW headers
#ifdef _WIN32
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#else
#include <glad/glad.h>
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace std;
// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes

void QueryGLVersion();
bool CheckGLErrors();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

enum Effect {
   NO_EFFECT = 0,
   EFFECT1,
   EFFECT2,
   EFFECT3,
   EFFECT4,
};

// Current image state variables
static int currImageNum_ = 0;
static string currImageFileName_ = "images/image1-mandrill.png";
static string currShaderFileName_ = "colourFragment.glsl";

// State per image
static vector<Effect> colourEffects_;
static vector<Effect> filters_;
static vector<Effect> blurs_;

// Current global state variables
static double prevCoords_[2];
static bool shaderChanged_ = true;
static bool isDragging_ = false;

// Vertices
static vector<GLfloat> vertices;

// --------------------------------------------------------------------------
// Functions to set up OpenGL shader programs for rendering

struct MyShader
{
   // OpenGL names for vertex and fragment shaders, shader program
   GLuint  vertex;
   GLuint  fragment;
   GLuint  program;

   // initialize shader and program names to zero (OpenGL reserved value)
   MyShader() : vertex(0), fragment(0), program(0)
   {}
};

// load, compile, and link shaders, returning true if successful
bool InitializeShaders(MyShader *shader, string shaderName)
{
   // load shader source from files
   string vertexSource = LoadSource("vertex.glsl");
   string fragmentSource = LoadSource(shaderName);
   if (vertexSource.empty() || fragmentSource.empty()) return false;

   // compile shader source into shader objects
   shader->vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
   shader->fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

   // link shader program
   shader->program = LinkProgram(shader->vertex, shader->fragment);

   // check for OpenGL errors and return false if error occurred
   return !CheckGLErrors();
}

// deallocate shader-related objects
void DestroyShaders(MyShader *shader)
{
   // unbind any shader programs and destroy shader objects
   glUseProgram(0);
   glDeleteProgram(shader->program);
   glDeleteShader(shader->vertex);
   glDeleteShader(shader->fragment);
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing textures

struct MyTexture
{
   GLuint textureID;
   GLuint target;
   int width;
   int height;

   // initialize object names to zero (OpenGL reserved value)
   MyTexture() : textureID(0), target(0), width(0), height(0)
   {}
};

bool InitializeTexture(MyTexture* texture, const char* filename, GLuint target = GL_TEXTURE_2D)
{
   int numComponents;
   stbi_set_flip_vertically_on_load(true);
   unsigned char *data = stbi_load(filename, &texture->width, &texture->height, &numComponents, 0);
   if (data != nullptr)
   {
      texture->target = target;
      glEnable(texture->target);
      glGenTextures(1, &texture->textureID);
      glBindTexture(texture->target, texture->textureID);
      GLuint format = numComponents == 3 ? GL_RGB : GL_RGBA;
      glTexImage2D(texture->target, 0, format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, data);

      // Note: Only wrapping modes supported for GL_TEXTURE_RECTANGLE when defining
      // GL_TEXTURE_WRAP are GL_CLAMP_TO_EDGE or GL_CLAMP_TO_BORDER
      glTexParameteri(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      // Clean up
      glBindTexture(texture->target, 0);
      stbi_image_free(data);
      return !CheckGLErrors();
   }
   return true; //error
}

// deallocate texture-related objects
void DestroyTexture(MyTexture *texture)
{
   glBindTexture(texture->target, 0);
   glDeleteTextures(1, &texture->textureID);
}

void SaveImage(const char* filename, int width, int height, unsigned char *data, int numComponents = 3, int stride = 0)
{
   if (!stbi_write_png(filename, width, height, numComponents, data, stride))
      cout << "Unable to save image: " << filename << endl;
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct MyGeometry
{
   // OpenGL names for array buffer objects, vertex array object
   GLuint  vertexBuffer;
   GLuint  textureBuffer;
   GLuint  colourBuffer;
   GLuint  vertexArray;
   GLsizei elementCount;

   // initialize object names to zero (OpenGL reserved value)
   MyGeometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0)
   {}
};

void createImageWithAspectRatio(MyTexture& texture)
{
   GLfloat height = 1.0;
   GLfloat width = 1.0;

   if (texture.width > texture.height)
   {
      GLfloat ratio = static_cast<GLfloat>(texture.height) / static_cast<GLfloat>(texture.width);
      height *= ratio;
   }
   else
   {
      GLfloat ratio = static_cast<GLfloat>(texture.width) / static_cast<GLfloat>(texture.height);
      width *= ratio;
   }

   // Initialize two triangles with the aspect ratio
   vertices[0] = (-1.0f * width);
   vertices[1] = (-1.0f * height);
   vertices[2] = (-1.0f * width);
   vertices[3] = (1.0f * height);
   vertices[4] = (1.0f * width);
   vertices[5] = (1.0f * height);
   vertices[6] = (1.0f * width);
   vertices[7] = (1.0f * height);
   vertices[8] = (1.0f * width);
   vertices[9] = (-1.0f * height);
   vertices[10] = (-1.0f * width);
   vertices[11] = (-1.0f * height);
}

// create buffers and fill with geometry data, returning true if successful
bool InitializeGeometry(MyGeometry *geometry, MyTexture* texture)
{
   // three vertex positions and associated textures of a triangle
   const GLfloat colours[][3] = {
      { 1.0f, 0.0f, 0.0f },
      { 0.0f, 1.0f, 0.0f },
      { 0.0f, 0.0f, 1.0f }
   };

   // Map texture coordinates to the geometry coordinates
   vector<GLfloat> textures;
   textures.push_back(0.0f);
   textures.push_back(0.0f);
   textures.push_back(0.0f);
   textures.push_back(static_cast<GLfloat>(texture->height));
   textures.push_back(static_cast<GLfloat>(texture->width));
   textures.push_back(static_cast<GLfloat>(texture->height));
   textures.push_back(static_cast<GLfloat>(texture->width));
   textures.push_back(static_cast<GLfloat>(texture->height));
   textures.push_back(static_cast<GLfloat>(texture->width));
   textures.push_back(0.0f);
   textures.push_back(0.0f);
   textures.push_back(0.0f);

   geometry->elementCount = vertices.size() / 2;

   // these vertex attribute indices correspond to those specified for the
   // input variables in the vertex shader
   const GLuint VERTEX_INDEX = 0;
   const GLuint COLOUR_INDEX = 1;
   const GLuint TEXTURE_INDEX = 2;

   // create an array buffer object for storing our vertices
   glGenBuffers(1, &geometry->vertexBuffer);
   glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
   glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

   // create an array buffer object for storing our textures
   glGenBuffers(1, &geometry->textureBuffer);
   glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
   glBufferData(GL_ARRAY_BUFFER, textures.size()*sizeof(GLfloat), textures.data(), GL_STATIC_DRAW);

   // create another one for storing our colours
   glGenBuffers(1, &geometry->colourBuffer);
   glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

   // create a vertex array object encapsulating all our vertex attributes
   glGenVertexArrays(1, &geometry->vertexArray);
   glBindVertexArray(geometry->vertexArray);

   // associate the position array with the vertex array object
   glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
   glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glEnableVertexAttribArray(VERTEX_INDEX);

   // associate the texture array with the vertex array object
   glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
   glVertexAttribPointer(TEXTURE_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
   glEnableVertexAttribArray(TEXTURE_INDEX);

   // associate the colour array with the vertex array object
   glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
   glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
   glEnableVertexAttribArray(COLOUR_INDEX);

   // unbind our buffers, resetting to default state
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindVertexArray(0);

   // check for OpenGL errors and return false if error occurred
   return !CheckGLErrors();
}

// deallocate geometry-related objects
void DestroyGeometry(MyGeometry *geometry)
{
   // unbind and destroy our vertex array object and associated buffers
   glBindVertexArray(0);
   glDeleteVertexArrays(1, &geometry->vertexArray);
   glDeleteBuffers(1, &geometry->vertexBuffer);
   glDeleteBuffers(1, &geometry->colourBuffer);
   glDeleteBuffers(1, &geometry->textureBuffer);
}

// --------------------------------------------------------------------------
// Rendering function that draws our scene to the frame buffer

void RenderScene(MyGeometry *geometry, MyTexture* texture, MyShader *shader)
{
   // clear screen to a dark grey colour
   glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT);

   // bind our shader program and the vertex array object containing our
   // scene geometry, then tell OpenGL to draw our geometry
   glUseProgram(shader->program);
   glBindVertexArray(geometry->vertexArray);
   glBindTexture(texture->target, texture->textureID);
   glDrawArrays(GL_TRIANGLES, 0, geometry->elementCount);

   // reset state to default (no shader or geometry bound)
   glBindTexture(texture->target, 0);
   glBindVertexArray(0);
   glUseProgram(0);

   // check for an report any OpenGL errors
   CheckGLErrors();
}

// --------------------------------------------------------------------------
// GLFW callback functions

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
   cout << "GLFW ERROR " << error << ":" << endl;
   cout << description << endl;
}

// handles keyboard input events
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
   {
      glfwSetWindowShouldClose(window, GL_TRUE);
   }
   else if (key == GLFW_KEY_1 && action == GLFW_PRESS)
   {
      currImageNum_ = 0;
      currImageFileName_ = "images/image1-mandrill.png";
   }
   else if (key == GLFW_KEY_2  && action == GLFW_PRESS)
   {
      currImageNum_ = 1;
      currImageFileName_ = "images/image2-uclogo.png";
   }
   else if (key == GLFW_KEY_3  && action == GLFW_PRESS)
   {
      currImageNum_ = 2;
      currImageFileName_ = "images/image3-aerial.jpg";
   }
   else if (key == GLFW_KEY_4  && action == GLFW_PRESS)
   {
      currImageNum_ = 3;
      currImageFileName_ = "images/image4-thirsk.jpg";
   }
   else if (key == GLFW_KEY_5  && action == GLFW_PRESS)
   {
      currImageNum_ = 4;
      currImageFileName_ = "images/image5-pattern.png";
   }
   else if (key == GLFW_KEY_6  && action == GLFW_PRESS)
   {
      currImageNum_ = 5;
      currImageFileName_ = "images/image6-edc2016.jpg";
   }
   else if (key == GLFW_KEY_C && action == GLFW_PRESS)
   {
      shaderChanged_ = true;
      currShaderFileName_ = "colourFragment.glsl";
      colourEffects_[currImageNum_] = static_cast<Effect>((colourEffects_[currImageNum_] + 1) % 5);
   }
   else if (key == GLFW_KEY_F && action == GLFW_PRESS)
   {
      shaderChanged_ = true;
      currShaderFileName_ = "filterFragment.glsl";
      filters_[currImageNum_] = static_cast<Effect>((filters_[currImageNum_] + 1) % 4);
   }
   else if (key == GLFW_KEY_B && action == GLFW_PRESS)
   {
      shaderChanged_ = true;
      currShaderFileName_ = "blurFragment.glsl";
      blurs_[currImageNum_] = static_cast<Effect>((blurs_[currImageNum_] + 1) % 4);
   }
   else if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
   {
      GLfloat angle  = static_cast<GLfloat>(M_PI / -8);

      // Apply rotation matrix to each points x & y
      for (unsigned int i = 0; i < vertices.size(); i += 2)
      {
         float x = vertices[i] * cos(angle) - vertices[i + 1] * sin(angle);
         float y = vertices[i + 1] * cos(angle) + vertices[i] * sin(angle);
         vertices[i] = x;
         vertices[i + 1] = y;
      }
   }
   else if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
   {
      GLfloat angle = static_cast<GLfloat>(M_PI / 8);

      // Apply rotation matrix to each points x & y
      for (unsigned int i = 0; i < vertices.size(); i += 2)
      {
         float x = vertices[i] * cos(angle) - vertices[i + 1] * sin(angle);
         float y = vertices[i + 1] * cos(angle) + vertices[i] * sin(angle);
         vertices[i] = x;
         vertices[i + 1] = y;
      }
   }
}

// handles mouse input events
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
   if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
   {
      isDragging_ = true;

      // Update previous cursor position
      double xPos, yPos;
      glfwGetCursorPos(window, &xPos, &yPos);
      prevCoords_[0] = xPos;
      prevCoords_[1] = yPos;
   }
   else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
   {
      isDragging_ = false;
   }
}

// handles cursor position events
void CursorPosCallback(GLFWwindow* window, double xPos, double yPos)
{
   if (isDragging_)
   {
      // Amount mouse has moved, normalized
      for (unsigned int i = 0; i < vertices.size(); i += 2)
      {
         vertices[i] += static_cast<GLfloat>(2 * (xPos - prevCoords_[0]) / 512);
         vertices[i + 1] -= static_cast<GLfloat>(2 * (yPos - prevCoords_[1]) / 512);
      }
   }

   prevCoords_[0] = xPos;
   prevCoords_[1] = yPos;
}

// handles scroll events
void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
   // Don't zoom if dragging
   if (isDragging_)
   {
      return;
   }

   float zoom = 100.0f;
   zoom += static_cast<GLfloat>(yOffset*2);

   for (unsigned int i = 0; i < vertices.size(); i += 2)
   {
      vertices[i] = vertices[i] * zoom / 100.0f;
      vertices[i + 1] = vertices[i + 1] * zoom / 100.0f;
   }
}

// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{
   // initialize the GLFW windowing system
   if (!glfwInit()) {
      cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
      return -1;
   }
   glfwSetErrorCallback(ErrorCallback);

   // attempt to create a window with an OpenGL 4.1 core profile context
   GLFWwindow *window = 0;
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   window = glfwCreateWindow(512, 512, "CPSC 453 OpenGL Assignment 2", 0, 0);
   if (!window) {
      cout << "Program failed to create GLFW window, TERMINATING" << endl;
      glfwTerminate();
      return -1;
   }

   // set keyboard & mouse callback functions and make our context current (active)
   glfwSetKeyCallback(window, KeyCallback);
   glfwSetMouseButtonCallback(window, MouseButtonCallback);
   glfwSetCursorPosCallback(window, CursorPosCallback);
   glfwSetScrollCallback(window, ScrollCallback);
   glfwMakeContextCurrent(window);

   //Initialize GLAD
   if (!gladLoadGL())
   {
      cout << "GLAD init failed" << endl;
      return -1;
   }

   // query and print out information about our OpenGL environment
   QueryGLVersion();

   // call function to load and compile shader programs
   MyShader shader;
   MyTexture texture;
   MyGeometry geometry;

   // Initialize each images state variables
   for (int i = 0; i < 6; i++)
   {
      colourEffects_.push_back(NO_EFFECT);
      filters_.push_back(NO_EFFECT);
      blurs_.push_back(NO_EFFECT);
      vertices.push_back(0.0); //x
      vertices.push_back(0.0); //y
   }

   // Variable to check if image has changed
   int prevImage = -1;

   // run an event-triggered main loop
   while (!glfwWindowShouldClose(window))
   {
      // reset
      DestroyGeometry(&geometry);

      if (shaderChanged_)
      {
         shaderChanged_ = false;
         if (!InitializeShaders(&shader, currShaderFileName_)) {
            cout << "Program could not initialize shaders, TERMINATING" << endl;
            return -1;
         }
      }

      if (currImageNum_ != prevImage)
      {
         prevImage = currImageNum_;
         if (!InitializeTexture(&texture, currImageFileName_.c_str(), GL_TEXTURE_RECTANGLE))
            cout << "Program failed to initialize texture!" << endl;
         createImageWithAspectRatio(texture);
      }

      // call function to create and fill buffers with geometry data
      if (!InitializeGeometry(&geometry, &texture))
         cout << "Program failed to initialize geometry!" << endl;

      glUseProgram(shader.program);
      GLuint colourEffectUniform = glGetUniformLocation(shader.program, "colourEffect");
      GLuint filterUniform = glGetUniformLocation(shader.program, "filter");
      GLuint blurUniform = glGetUniformLocation(shader.program, "blur");
      glUniform1i(colourEffectUniform, colourEffects_.at(currImageNum_));
      glUniform1i(filterUniform, filters_.at(currImageNum_));
      glUniform1i(blurUniform, blurs_.at(currImageNum_));

      // call function to draw our scene
      RenderScene(&geometry, &texture, &shader); //render scene with texture

      glfwSwapBuffers(window);

      glfwPollEvents();
   }

   // clean up allocated resources before exit
   DestroyTexture(&texture);
   DestroyGeometry(&geometry);
   DestroyShaders(&shader);
   glfwDestroyWindow(window);
   glfwTerminate();

   cout << "Goodbye!" << endl;
   return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
   // query opengl version and renderer information
   string version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
   string glslver = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
   string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

   cout << "OpenGL [ " << version << " ] "
      << "with GLSL [ " << glslver << " ] "
      << "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors()
{
   bool error = false;
   for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
   {
      cout << "OpenGL ERROR:  ";
      switch (flag) {
      case GL_INVALID_ENUM:
         cout << "GL_INVALID_ENUM" << endl; break;
      case GL_INVALID_VALUE:
         cout << "GL_INVALID_VALUE" << endl; break;
      case GL_INVALID_OPERATION:
         cout << "GL_INVALID_OPERATION" << endl; break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:
         cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
      case GL_OUT_OF_MEMORY:
         cout << "GL_OUT_OF_MEMORY" << endl; break;
      default:
         cout << "[unknown error code]" << endl;
      }
      error = true;
   }
   return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
   string source;

   ifstream input(filename.c_str());
   if (input) {
      copy(istreambuf_iterator<char>(input),
         istreambuf_iterator<char>(),
         back_inserter(source));
      input.close();
   }
   else {
      cout << "ERROR: Could not load shader source from file "
         << filename << endl;
   }

   return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
   // allocate shader object name
   GLuint shaderObject = glCreateShader(shaderType);

   // try compiling the source as a shader of the given type
   const GLchar *source_ptr = source.c_str();
   glShaderSource(shaderObject, 1, &source_ptr, 0);
   glCompileShader(shaderObject);

   // retrieve compile status
   GLint status;
   glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
   if (status == GL_FALSE)
   {
      GLint length;
      glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
      string info(length, ' ');
      glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
      cout << "ERROR compiling shader:" << endl << endl;
      cout << source << endl;
      cout << info << endl;
   }

   return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
   // allocate program object name
   GLuint programObject = glCreateProgram();

   // attach provided shader objects to this program
   if (vertexShader)   glAttachShader(programObject, vertexShader);
   if (fragmentShader) glAttachShader(programObject, fragmentShader);

   // try linking the program with given attachments
   glLinkProgram(programObject);

   // retrieve link status
   GLint status;
   glGetProgramiv(programObject, GL_LINK_STATUS, &status);
   if (status == GL_FALSE)
   {
      GLint length;
      glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
      string info(length, ' ');
      glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
      cout << "ERROR linking shader program:" << endl;
      cout << info << endl;
   }

   return programObject;
}