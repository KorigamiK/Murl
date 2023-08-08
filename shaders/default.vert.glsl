attribute vec3 aPos;
uniform mat4 transform;
varying vec4 FragColor;

void main() {
  gl_Position = transform * vec4(aPos, 1.0);
  FragColor = vec4(1.0, 0.5, 0.2, 1.0);
}
