uniform float time;

void main() {
  gl_FragColor = vec4(1.0 * sin(time), 0.5 * cos(time), 0.2 * sin(time), 1.0);
}
