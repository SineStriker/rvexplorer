project(bintoh++ LANGUAGES CXX)

file(GLOB _src *.h *.cpp)

add_executable(${PROJECT_NAME} ${_src})