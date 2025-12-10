cmake_minimum_required(VERSION 3.25)

project("snake" VERSION 0.1)

add_executable(${PROJECT_NAME} WIN32
	"source/main.cpp"
	"source/resource/resource.rc"
)
target_link_libraries(${PROJECT_NAME} PUBLIC "engine")

install(TARGETS ${PROJECT_NAME})
