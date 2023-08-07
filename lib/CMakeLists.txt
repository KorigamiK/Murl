project(
	LibMurl
	VERSION 0.1.0
)

file(GLOB_RECURSE SOURCES "src/*.cc")
file(GLOB_RECURSE HEADERS "include/*.hh")

message(STATUS "Sources: ${SOURCES}")
message(STATUS "Headers: ${HEADERS}")

if(EMSCRIPTEN)
	add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})
else()
	add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})
endif()

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