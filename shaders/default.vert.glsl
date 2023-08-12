#version 300 es

precision mediump float;

in vec3 aPos;
uniform mat4 transform;
out vec4 FragColor;

void main() {
  gl_Position = transform * vec4(aPos, 1.0f);
  FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
