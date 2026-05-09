cmake_minimum_required(VERSION 3.25)

project("simple")

add_executable(${PROJECT_NAME} WIN32
	"source/main.cpp"
	"source/main.hpp")
target_link_libraries(${PROJECT_NAME} PUBLIC "engine")

install(TARGETS ${PROJECT_NAME})
