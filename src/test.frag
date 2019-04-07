/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 fragColour;
layout(location = 0) out vec4 outColour;

void main() {
  outColour = fragColour;
}
