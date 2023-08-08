#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "SDL_video.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define exit(code) emscripten_force_exit(code)
#endif

const char* vertexShaderPath = "shaders/default.vert.glsl";
const char* fragmentShaderPath = "shaders/default.frag.glsl";

unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

// Function to read shader code from a file
std::string ReadShaderCode(const char* filePath) {
  std::ifstream shaderFile(filePath);
  if (!shaderFile.is_open()) {
    std::cerr << "Failed to open shader file: " << filePath << std::endl;
    exit(EXIT_FAILURE);
  }

  std::stringstream shaderStream;
  shaderStream << shaderFile.rdbuf();
  shaderFile.close();

  return shaderStream.str();
}

// Compile shader and return shader ID
GLuint CompileShader(GLenum shaderType, const char* shaderCode) {
  GLuint shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, &shaderCode, nullptr);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    GLint logSize;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
    std::vector<GLchar> errorLog(logSize);
    glGetShaderInfoLog(shader, logSize, nullptr, errorLog.data());
    std::cerr << "Shader compilation error:\n" << shaderCode << std::endl << errorLog.data() << std::endl;
    glDeleteShader(shader);
    exit(EXIT_FAILURE);
  }

  return shader;
}

// Create shader program and return program ID
GLuint CreateShaderProgram(const char* vertexCode, const char* fragmentCode) {
  GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexCode);
  GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentCode);

  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    GLint logSize;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
    std::vector<GLchar> errorLog(logSize);
    glGetProgramInfoLog(program, logSize, nullptr, errorLog.data());

    std::cerr << "Shader program linking error:\n" << errorLog.data() << std::endl;
    glDeleteProgram(program);
    exit(EXIT_FAILURE);
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return program;
}

static GLuint shaderProgram;
static GLuint VBO, VAO;
static void* mainLoopArg;
static SDL_Window* window;

// Matrices for transformation
glm::mat4 transform = glm::mat4(1.0f);
bool quit = false;

void mainLoop(void* arg) {
  (void)arg;
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        quit = true;
        break;
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
            quit = true;
            break;
        }
        break;
      case SDL_WINDOWEVENT:
        switch (event.window.event) {
          case SDL_WINDOWEVENT_RESIZED:
            glViewport(0, 0, event.window.data1, event.window.data2);
            SCR_WIDTH = event.window.data1;
            SCR_HEIGHT = event.window.data2;
            break;
        }
      default:
        break;
    }
  }

  // Update transform matrix based on mouse position
  int mouseX, mouseY;
  SDL_GetMouseState(&mouseX, &mouseY);
  float xPos = static_cast<float>(mouseX) / static_cast<float>(SCR_WIDTH) * 2.0f - 1.0f;
  float yPos = -static_cast<float>(mouseY) / static_cast<float>(SCR_HEIGHT) * 2.0f + 1.0f;
  transform = glm::translate(glm::mat4(1.0f), glm::vec3(xPos, yPos, 0.0f));

  // Clear the screen
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Use shader program and set uniforms
  glUseProgram(shaderProgram);
  GLuint transformLoc = glGetUniformLocation(shaderProgram, "transform");
  glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

  // Draw the triangle
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);

  // Swap buffers
  SDL_GL_SwapWindow(window);
};

int main() {
  // Initialize SDL and create a window
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
    return EXIT_FAILURE;
  }

  window = SDL_CreateWindow("Hello Triangle", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCR_WIDTH, SCR_HEIGHT,
                            SDL_WINDOW_OPENGL);
  if (!window) {
    std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return EXIT_FAILURE;
  }

  // Create OpenGL context
  SDL_GLContext glContext = SDL_GL_CreateContext(window);
  if (!glContext) {
    std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_FAILURE;
  }

  // Initialize GLEW
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW" << std::endl;
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_FAILURE;
  }

  // Load and compile shaders
  std::string vertexShaderCode = ReadShaderCode(vertexShaderPath);
  std::string fragmentShaderCode = ReadShaderCode(fragmentShaderPath);
  shaderProgram = CreateShaderProgram(vertexShaderCode.c_str(), fragmentShaderCode.c_str());

  // Triangle vertices
  float vertices[] = {0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f};

  // Vertex buffer object (VBO) and vertex array object (VAO)
  glGenBuffers(1, &VBO);
  glGenVertexArrays(1, &VAO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

#ifdef __EMSCRIPTEN__
  int fps = 0;  // Use browser's requestAnimationFrame
  mainLoopArg = nullptr;
  emscripten_set_main_loop_arg(mainLoop, mainLoopArg, fps, true);
#else
  while (!quit) {
    mainLoop(nullptr);
  }
#endif

  // Cleanup
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteProgram(shaderProgram);

  SDL_GL_DeleteContext(glContext);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
