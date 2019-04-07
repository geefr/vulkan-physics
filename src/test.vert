/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#version 450

layout(location = 0) in vec4 vert_partPos;
layout(location = 1) in vec4 vert_colour;
layout(location = 2) in float vert_radius;

layout(location = 0) out vec4 fragColour;

layout(push_constant) uniform PushConstants {
  mat4 modelM;
  mat4 viewM;
  mat4 projM;
} push;

void main() {
  gl_Position = push.projM * push.viewM * push.modelM * vert_partPos;

  // Set the particle size based on dimensions
  gl_PointSize = 1;

  fragColour = vert_colour;

  //gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
  //fragColour = colours[gl_VertexIndex];
}
