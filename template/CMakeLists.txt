# A program that uses the canvas-cpp library (canvas, turtle, image, audio, chance, stats, stopwatch, input).
# Usually you only change the list of .cpp files in add_executable.

cmake_minimum_required(VERSION 3.24)
project(my_program CXX)

# Download the library (the first build takes a minute or two).
include(FetchContent)
FetchContent_Declare(canvas
  GIT_REPOSITORY https://github.com/irauf-iba/canvas-cpp
  GIT_TAG        v0.1   # the library version this project uses
  GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(canvas)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE canvas::canvas)

# Copy the data folder (images, sounds) next to the program after each build.
add_custom_command(TARGET main POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/data $<TARGET_FILE_DIR:main>)
