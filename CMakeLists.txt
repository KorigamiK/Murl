cmake_minimum_required(VERSION 3.14)

project(
	Murl
	VERSION 0.1.0
	DESCRIPTION "Music Uptake Rendering Library"
	HOMEPAGE_URL "https://github.com/korigamik/murl"
	LANGUAGES CXX
)

add_subdirectory(lib)

file(GLOB_RECURSE SOURCES "src/*.cc")

message(STATUS "Sources: ${SOURCES}")

# aux_source_directory(src/ SOURCES)

add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

if (EMSCRIPTEN)
	set(CMAKE_EXECUTABLE_SUFFIX ".html")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s FULL_ES3=1 -s USE_WEBGL2=1")
	# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sEXIT_RUNTIME")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --preload-file shaders")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WARN_ON_UNDEFINED_SYMBOLS=1 -sASSERTIONS=1")
	set(OPENGL_ES_2 TRUE)
	# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --preload-file ${CMAKE_SOURCE_DIR}/logo.png@/")

	target_include_directories(
		${PROJECT_NAME} 
		PUBLIC 
			${CMAKE_SOURCE_DIR}/include
	)

	target_link_libraries(
		${PROJECT_NAME} 
		PRIVATE 
			LibMurl 
	)

else()

	# Use pkg-config to find SDL2 and GLEW
	find_package(glm REQUIRED) # GL Mathematics (GLM)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(SDL2 REQUIRED sdl2) # SDL2
	pkg_check_modules(GLEW REQUIRED glew) # GL Extension Wrangler (GLEW)
  pkg_check_modules(SDL2_mixer REQUIRED SDL2_mixer) # SDL2_mixer

	message(STATUS "SDL2_INCLUDE_DIRS: ${SDL2_INCLUDE_DIRS}")
	message(STATUS "SDL2_LIBRARIES: ${SDL2_LIBRARIES}")
  message(STATUS "SDL2_mixer_INCLUDE_DIRS: ${SDL2_mixer_INCLUDE_DIRS}")
  message(STATUS "SDL2_mixer_LIBRARIES: ${SDL2_mixer_LIBRARIES}")
	message(STATUS "GLEW_INCLUDE_DIRS: ${GLEW_INCLUDE_DIRS}")
	message(STATUS "GLEW_LIBRARIES: ${GLEW_LIBRARIES}")
	message(STATUS "GLM_INCLUDE_DIRS: ${GLM_INCLUDE_DIRS}")

	target_include_directories(
		${PROJECT_NAME} 
		PUBLIC 
			${CMAKE_SOURCE_DIR}/include
		PRIVATE
			${SDL2_INCLUDE_DIRS}
      ${SDL2_mixer_INCLUDE_DIRS}
			${GLEW_INCLUDE_DIRS}
			${GLM_INCLUDE_DIRS}
	)

	target_link_libraries(
		${PROJECT_NAME} 
		PRIVATE 
			LibMurl 
		PUBLIC
			${SDL2_LIBRARIES}
      ${SDL2_mixer_LIBRARIES}
			${GLEW_LIBRARIES}
	)

	
	enable_testing()
	
endif()
	
# Symbolic link of shaders to build directory
execute_process(
	COMMAND ${CMAKE_COMMAND} -E create_symlink
	${CMAKE_CURRENT_SOURCE_DIR}/shaders
	${CMAKE_CURRENT_BINARY_DIR}/shaders
)

if (BUILD_TESTING)
	add_subdirectory(test)
endif()

set(GENERATE_DOXYFILE OFF CACHE BOOL "Generate documentation")


if(NOT Emscripten AND GENERATE_DOXYFILE)

	find_package(Doxygen REQUIRED)
	set(DOXYGEN_GENERATE_HTML YES)
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in # input
		${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile # output
		@ONLY
	)
	add_custom_target(
		doc
		COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs
		COMMENT "Generate API documentation with Doxygen"
		VERBATIM
	)

endif()
