cmake_minimum_required(VERSION 3.14)

project(
	murl
	VERSION 0.1.0
	DESCRIPTION "Music Uptake Rendering Library"
	HOMEPAGE_URL "https://github.com/korigamik/murl"
	LANGUAGES CXX
)

add_subdirectory(lib)

enable_testing()

set(BUILD_TESTING ON CACHE BOOL "Build tests")

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
