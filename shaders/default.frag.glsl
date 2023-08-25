#version 300 es

precision mediump float;

uniform float waveData[256];
uniform vec2 resolution;
uniform float time;

out vec4 FragColor;

#define THICKNESS 0.04

void interpolateValue(in float index, out float value) {
  float norm = 255.0f / resolution.x * index;
  int Floor = int(floor(norm));
  int Ceil = int(ceil(norm));
  value = mix(waveData[Floor], waveData[Ceil], fract(norm));
}

void main() {
  float x = gl_FragCoord.x / resolution.x;
  float y = gl_FragCoord.y / resolution.y;

  float wave = 0.0f;
  interpolateValue(x * resolution.x, wave);

  wave = 0.5f - wave / 3.0f; //centers wave

  float r = abs(THICKNESS / (wave - y));
  FragColor = vec4(r - abs(r * 0.9f * sin(time / 5.0f)), r - abs(r * 0.9f * sin(time / 7.0f)), r - abs(r * 0.9f * sin(time / 9.0f)), 0);
}
