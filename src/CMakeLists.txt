set( targetName "physics" )

include_directories(${PROJECT_SOURCE_DIR}/vulkanutils)

add_executable(${targetName}
  main.cpp
  vulkanapp.h
  vulkanapp.cpp
	)
target_link_libraries( ${targetName} Vulkan::Vulkan glfw vulkanutils )

add_custom_target(${targetName}-shader-vert
	COMMAND ${glslCompiler} -V ${CMAKE_CURRENT_SOURCE_DIR}/test.vert
	SOURCES test.vert )
add_custom_target(${targetName}-shader-frag
	COMMAND ${glslCompiler} -V ${CMAKE_CURRENT_SOURCE_DIR}/test.frag
	SOURCES test.frag )
add_custom_target(${targetName}-shader-comp
  COMMAND ${glslCompiler} -V ${CMAKE_CURRENT_SOURCE_DIR}/test.comp
  SOURCES test.comp )

add_dependencies( ${targetName} ${targetName}-shader-vert ${targetName}-shader-frag ${targetName}-shader-comp)
