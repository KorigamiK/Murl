#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "SDL_events.h"
#include "SDL_stdinc.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define exit(code) emscripten_force_exit(code)
#endif

// Audio sutff
constexpr int frequency = 22050;
constexpr Uint16 format = MIX_DEFAULT_FORMAT;  // almost always 16-bit(Uint16) audio
const constexpr int channels = 2;
const constexpr int chunksize = 512;

bool isPlaying = false;

using audioFormat = Uint16;
constexpr int bytesPerSample = sizeof(audioFormat) * channels;
const int audioBufferSize = chunksize * channels;  // NOTE: this is arbitrary, we can use any size we want

struct AudioBuffer {
  int samplesCounter = 0;
  std::array<float, audioBufferSize> buffer;
  std::mutex buffer_mutex;
};

AudioBuffer audioBuffer;

void audioCallback(void* userdata, Uint8* stream, int len) {
  /* One sample per frame for mono output, two samples per frame in a stereo setup, etc
   * Here len is just the number of frames that are determined by the chunksize = 8192
   * but to be sure we'll manually allocate the data instead.
   * So, 1 stream = 8192 bytes
   *              = 2048 samples
   *              = channels * format / 8
   *              = chunksize * channels * sizeof(Uint16)
   * This helped me https://developer.mozilla.org/en-US/docs/Web/Media/Formats/Audio_concepts#audio_channels_and_frames
   * */
  AudioBuffer* audio = static_cast<AudioBuffer*>(userdata);
  // copy the stream into the buffer as much as we can then overwrite the rest
  int samples = len / bytesPerSample;
  if (isPlaying) std::cout << "Audio callback called with samples: " << samples << std::endl;
  if (audio->samplesCounter + samples > audioBufferSize) {
    // buffer overflow, discard what ever is in the buffer and start over
    for (size_t n = 0; n < audioBufferSize; n++) audio->buffer[n] = 0;
    audio->samplesCounter = 0;
  }
  int n = 0;
  for (uint16_t i = 0; i < samples; i++) {
    switch (bytesPerSample) {
      case 4: {
        assert(channels == 2);  // only support stereo
        // only put left channel into the buffer
        int16_t* buf16 = (int16_t*)&stream[n];
        audio->buffer[i + audio->samplesCounter] = (float)*buf16 / (float)INT16_MAX;
      } break;
      default: {
        std::cerr << "Unsupported number of bytes per sample: " << bytesPerSample << std::endl;
      } break;
    }
    n += bytesPerSample;
  }
  audio->samplesCounter += samples;
  audio->buffer_mutex.unlock();
}

Mix_Music* music = nullptr;
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

const char* vertexShaderPath = "shaders/default.vert.glsl";
const char* fragmentShaderPath = "shaders/default.frag.glsl";

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
GLuint transformLoc, waveDataLoc, timeLoc;
static void* mainLoopArg;
static SDL_Window* window;

void loadShaders() {
  std::string vertexShaderCode = ReadShaderCode(vertexShaderPath);
  std::string fragmentShaderCode = ReadShaderCode(fragmentShaderPath);
  GLuint newShaderProgram = CreateShaderProgram(vertexShaderCode.c_str(), fragmentShaderCode.c_str());
  // destroy the old shader
  glDeleteProgram(shaderProgram);
  shaderProgram = newShaderProgram;
  transformLoc = glGetUniformLocation(shaderProgram, "transform");
  waveDataLoc = glGetUniformLocation(shaderProgram, "waveData");
  timeLoc = glGetUniformLocation(shaderProgram, "time");
}

// Matrices for transformation
glm::mat4 transform = glm::mat4(1.0f);
bool quit = false;

inline void mainLoop(void* arg) {
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
          case SDLK_SPACE:
            if (isPlaying)
              Mix_PauseMusic();
            else
              Mix_ResumeMusic();
            isPlaying = !isPlaying;
            break;
          case SDLK_r: {  // reload the shader
            std::cout << "Reloading shader" << std::endl;
            std::string vertexShaderCode = ReadShaderCode(vertexShaderPath);
            std::string fragmentShaderCode = ReadShaderCode(fragmentShaderPath);
            shaderProgram = CreateShaderProgram(vertexShaderCode.c_str(), fragmentShaderCode.c_str());
          } break;
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
      case SDL_DROPFILE:
        if (event.drop.type == SDL_DROPFILE) {
          std::cout << "File dropped: " << event.drop.file << std::endl;
          char* const droppedFilePath = event.drop.file;
          SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "File dropped on window", droppedFilePath, window);
          music = Mix_LoadMUS(droppedFilePath);
          if (music) {
            Mix_PlayMusic(music, -1);
            Mix_SetPostMix(audioCallback, static_cast<void*>(&audioBuffer));
            isPlaying = true;
            SDL_free(droppedFilePath);
          }
        }
        break;
      default:
        break;
    }
  }

  // Update the time
  float time = static_cast<float>(SDL_GetTicks()) / 1000.0f;
  glUniform1f(timeLoc, time);

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
  glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

  // print the buffer data
  if (isPlaying and audioBuffer.samplesCounter > 0) {
    std::cout << "Samples counter: " << audioBuffer.samplesCounter << std::endl;
    std::cout << "Buffer data: ";
    for (int i = 0; i < audioBuffer.samplesCounter; i++) {
      std::cout << audioBuffer.buffer[i] << " ";
    }
    std::cout << std::endl;
  }

  // Draw the triangle
  glBindVertexArray(VAO);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);

  // Swap buffers
  SDL_GL_SwapWindow(window);
};

int main(int argc, char* argv[]) {
  (void)argc, (void)argv;

  // Initialize SDL and create a window
  if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
    return EXIT_FAILURE;
  }

  SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

  // Initialize SDL2_mixer
  if (Mix_OpenAudio(frequency, format, channels, chunksize) < 0) {
    std::cerr << "Failed to initialize SDL2_mixer: " << SDL_GetError() << std::endl;
    return EXIT_FAILURE;
  }
  // print audio info
  // Mix_QuerySpec(&frequency, &format, &channels);
  std::cout << "Frequency: " << frequency << std::endl;
  std::cout << "Format: " << format << std::endl;
  std::cout << "Channels: " << channels << std::endl;
  std::cout << "Chunksize: " << chunksize << std::endl;
  std::cout << "Bytes per sample: " << bytesPerSample << std::endl;
  std::cout << "Audio Buffer size: " << audioBufferSize << std::endl;

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

  int glVersion[2] = {-1, 1};
  glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
  glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);

  std::cerr << "Using OpenGL: " << glVersion[0] << "." << glVersion[1] << std::endl;
  std::cerr << "Renderer used: " << glGetString(GL_RENDERER) << std::endl;
  std::cerr << "Shading Language: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

  // Load and compile shaders
  loadShaders();

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

  Mix_HaltMusic();
  Mix_FreeMusic(music);
  Mix_CloseAudio();
  Mix_Quit();

  SDL_GL_DeleteContext(glContext);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
