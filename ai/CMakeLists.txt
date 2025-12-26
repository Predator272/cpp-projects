cmake_minimum_required(VERSION 3.25)

project("simple" VERSION 0.1)

add_executable(${PROJECT_NAME} "source/main.cpp" "source/ai.cpp")
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

install(TARGETS ${PROJECT_NAME})
