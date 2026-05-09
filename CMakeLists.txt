cmake_minimum_required(VERSION 3.25)

project("engine" VERSION 0.1)

add_library(${PROJECT_NAME} STATIC
	"source/common/main.cpp"
	"source/error/error.cpp"
	"source/error/crash_handler.cpp"
	"source/application/window.cpp"
	"source/application/application.cpp"
)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
target_include_directories(${PROJECT_NAME} PUBLIC "include" PRIVATE "source")
target_compile_definitions(${PROJECT_NAME} PUBLIC _UNICODE UNICODE)

install(TARGETS ${PROJECT_NAME})

add_subdirectory("examples")
