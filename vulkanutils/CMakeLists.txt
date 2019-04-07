set( targetName "vulkanutils" )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR}/util )

add_library( vulkanutils ${VULKANUTILS_LIB_TYPE}
  util/simplebuffer.h
  util/simplebuffer.cpp
  util/util.h
  util/util.cpp
  util/windowintegration.h
  util/windowintegration.cpp
  util/framebuffer.h
  util/framebuffer.cpp
  util/deviceinstance.h
  util/deviceinstance.cpp

  util/pipelines/pipeline.h
  util/pipelines/pipeline.cpp
  util/pipelines/graphicspipeline.h
  util/pipelines/graphicspipeline.cpp
  util/pipelines/computepipeline.h
  util/pipelines/computepipeline.cpp
	)
target_link_libraries( ${targetName} Vulkan::Vulkan glfw )

