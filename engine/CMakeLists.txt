cmake_minimum_required(VERSION 3.25)

project("engine" VERSION 0.1)

add_library(${PROJECT_NAME} STATIC
	"source/engine.cpp"
	"source/common.cpp"
	"source/error.cpp"
	"source/window.cpp"
)
target_include_directories(${PROJECT_NAME} PUBLIC "include")
target_compile_definitions(${PROJECT_NAME} PUBLIC _UNICODE UNICODE)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

install(TARGETS ${PROJECT_NAME})
