add_executable(test-elf ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

# target_compile_options(test-elf PRIVATE -static)
target_link_options(test-elf PRIVATE -static)
