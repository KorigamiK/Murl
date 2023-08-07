attribute vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
varying vec4 FragColor;

void main() {
  gl_Position = projection * view * model * vec4(aPos, 1.0);
  FragColor = vec4(1.0, 0.5, 0.2, 1.0);
}
