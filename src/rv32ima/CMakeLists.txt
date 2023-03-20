project(rv32ima LANGUAGES CXX)

file(GLOB _src *.h *.cpp)

get_filename_component(_name ${RV32IMA_BINARY_FILE} NAME_WLE)

add_executable(${PROJECT_NAME} ${_src})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Add implementation
set(RV32IMA_GENERATED_SOURCE_FILE ${CMAKE_CURRENT_BINARY_DIR}/${_name}.cpp)
file(WRITE ${RV32IMA_GENERATED_SOURCE_FILE} "")
add_custom_target(gen_run
    COMMAND $<TARGET_FILE:expander> ${RV32IMA_BINARY_FILE} ${RV32IMA_GENERATED_SOURCE_FILE}
)
add_dependencies(${PROJECT_NAME} gen_run)
target_sources(${PROJECT_NAME} PRIVATE ${RV32IMA_GENERATED_SOURCE_FILE})

# Add header
set(RV32IMA_HEADER_FILE ${CMAKE_BINARY_DIR}/include_temp/binary_data.h)
file(WRITE ${RV32IMA_HEADER_FILE} "const unsigned char binary_data[] = \"1\";")
add_custom_target(gen_header DEPENDS bintoh++
    COMMAND $<TARGET_FILE:bintoh++> ${RV32IMA_BINARY_FILE} ${RV32IMA_HEADER_FILE}
)
add_dependencies(${PROJECT_NAME} gen_header)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/include_temp)