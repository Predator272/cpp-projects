cmake_minimum_required(VERSION 3.25)

project("sysmgr" VERSION 0.1.0.0)

add_executable(${PROJECT_NAME} WIN32
	"source/main.cpp"
	"source/resources/version.rc"
	"source/resources/resources.rc"
)
target_compile_definitions(${PROJECT_NAME} PUBLIC _UNICODE UNICODE)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

install(TARGETS ${PROJECT_NAME})
