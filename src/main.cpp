/*
 * Provided under the BSD 3-Clause License, see LICENSE.
 *
 * Copyright (c) 2019, Gareth Francis
 * All rights reserved.
 */

#include "vulkanapp.h"

#include <exception>
#include <iostream>

int main(int argc, char* argv[])
{
  try {
    VulkanApp app;
    app.run();
  } catch ( std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}
