# Tests run without a display: drawing tests use the library's headless mode,
# and window and input tests use SDL's offscreen video driver.

if(MSVC)
  set(DRAW_TEST_WARNINGS /W4)
else()
  set(DRAW_TEST_WARNINGS -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

function(draw_test_executable name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE draw::draw ${ARGN})
  target_compile_options(${name} PRIVATE ${DRAW_TEST_WARNINGS})
endfunction()

# The public header compiles cleanly with strict warnings treated as errors.
draw_test_executable(header_test)
target_compile_options(header_test PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/WX,-Werror>)
add_test(NAME header COMMAND header_test)

# Snapshot tests against images in tests/reference.
draw_test_executable(render_test)
target_include_directories(render_test SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/stb)
foreach(scene IN ITEMS shapes polygons text pictures clipping aspect)
  add_test(NAME render.${scene}
           COMMAND render_test ${scene} ${CMAKE_CURRENT_SOURCE_DIR}/reference)
  set_tests_properties(render.${scene} PROPERTIES ENVIRONMENT DRAW_HEADLESS=1)
endforeach()

# Mouse and keyboard input, frame boundaries and pause() timing.
draw_test_executable(input_test SDL3::SDL3)
add_test(NAME input COMMAND input_test)
set_tests_properties(input PROPERTIES ENVIRONMENT SDL_VIDEO_DRIVER=offscreen TIMEOUT 30)

# Window lifetime.
draw_test_executable(window_test SDL3::SDL3)
add_test(NAME window.keep-open COMMAND window_test keep-open)
set_tests_properties(window.keep-open PROPERTIES PASS_REGULAR_EXPRESSION "PASSED")
foreach(name IN ITEMS close-in-pause close-while-drawing)
  add_test(NAME window.${name} COMMAND window_test ${name})
  set_tests_properties(window.${name} PROPERTIES FAIL_REGULAR_EXPRESSION "FAILED")
endforeach()
set_tests_properties(window.keep-open window.close-in-pause window.close-while-drawing
                     PROPERTIES ENVIRONMENT SDL_VIDEO_DRIVER=offscreen TIMEOUT 10)

# Each mistake stops the program with a helpful message.
draw_test_executable(error_test)
set(error_cases
  "nan|circle: an argument is NaN or infinite"
  "negative-radius|filledCircle: radius must not be negative"
  "missing-picture|picture: cannot open 'no-such-file.png'"
  "bad-save-format|save: 'out.gif' must end in .png, .jpg or .bmp"
  "size-mismatch|polygon: x and y must have the same size"
  "no-key|nextKeyTyped: no key was typed"
  "empty-scale|setXscale: min and max must be different"
  "bad-canvas-size|setCanvasSize: width and height must be positive"
  "missing-font|setFont: cannot open 'no-such-font.ttf'"
  "negative-pause|pause: ms must not be negative"
)
foreach(entry IN LISTS error_cases)
  string(FIND "${entry}" "|" bar)
  string(SUBSTRING "${entry}" 0 ${bar} name)
  math(EXPR start "${bar} + 1")
  string(SUBSTRING "${entry}" ${start} -1 message)
  string(REGEX REPLACE "([][.*+?()^$])" "\\\\\\1" pattern "draw: ${message}")
  add_test(NAME error.${name} COMMAND error_test ${name})
  set_tests_properties(error.${name} PROPERTIES
                       ENVIRONMENT DRAW_HEADLESS=1 PASS_REGULAR_EXPRESSION "${pattern}")
endforeach()

# Timings; build with `--target benchmark`, not run by ctest.
draw_test_executable(benchmark)
set_target_properties(benchmark PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Real keyboard and mouse input, driven by manual/real_input.sh with xdotool.
# Needs a desktop and takes keyboard focus, so it is not run by ctest.
add_executable(real_input manual/real_input.cpp)
target_link_libraries(real_input PRIVATE draw::draw)
target_compile_options(real_input PRIVATE ${DRAW_TEST_WARNINGS})
set_target_properties(real_input PROPERTIES EXCLUDE_FROM_ALL TRUE)
