project(
	LibMurl
	VERSION 0.1.0
)

file(GLOB_RECURSE SOURCES "src/*.cc")
file(GLOB_RECURSE HEADERS "include/*.hh")

message(STATUS "Sources: ${SOURCES}")
message(STATUS "Headers: ${HEADERS}")

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_include_directories(
	${PROJECT_NAME}
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(
	${PROJECT_NAME}
	PRIVATE
		-Wall
		-Wextra
		-Wpedantic
		-Werror
)

target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE
		${DEFINES}
)

# Symbolic link of shaders to build directory
execute_process(
	COMMAND ${CMAKE_COMMAND} -E create_symlink
	${CMAKE_CURRENT_SOURCE_DIR}/shaders
	${CMAKE_CURRENT_BINARY_DIR}/shaders
)