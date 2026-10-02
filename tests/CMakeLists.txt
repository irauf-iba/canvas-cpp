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

# The public headers compile cleanly, each on its own, with strict warnings
# treated as errors.
draw_test_executable(header_test)
target_sources(header_test PRIVATE header_color_test.cpp header_image_test.cpp header_audio_test.cpp)
target_compile_options(header_test PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/WX,-Werror>)
add_test(NAME header COMMAND header_test)

# Snapshot tests against images in tests/reference.
draw_test_executable(render_test)
target_include_directories(render_test SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/stb)
foreach(scene IN ITEMS shapes polygons text pictures clipping aspect image)
  add_test(NAME render.${scene}
           COMMAND render_test ${scene} ${CMAKE_CURRENT_SOURCE_DIR}/reference)
  set_tests_properties(render.${scene} PROPERTIES ENVIRONMENT DRAW_HEADLESS=1)
endforeach()

# The image module and the draw <-> image bridge.
draw_test_executable(image_test)
add_test(NAME image COMMAND image_test)
set_tests_properties(image PROPERTIES ENVIRONMENT DRAW_HEADLESS=1)

# The audio module without a device: files, resampling, clipping, capture.
draw_test_executable(audio_test)
add_test(NAME audio COMMAND audio_test ${CMAKE_CURRENT_SOURCE_DIR}/data)
set_tests_properties(audio PROPERTIES
                     ENVIRONMENT "DRAW_HEADLESS=1;DRAW_AUDIO_CAPTURE=audio_test_capture.wav")

# The audio module with SDL's dummy (real-time, silent) and disk drivers.
draw_test_executable(audio_device_test SDL3::SDL3)
foreach(name IN ITEMS pacing file background)
  add_test(NAME audio.${name} COMMAND audio_device_test ${name})
endforeach()
add_test(NAME audio.exit-drain COMMAND audio_device_test exit-drain)
set_tests_properties(audio.exit-drain PROPERTIES PASS_REGULAR_EXPRESSION "PASSED")
set_tests_properties(audio.pacing audio.file audio.background audio.exit-drain PROPERTIES
                     ENVIRONMENT "SDL_AUDIO_DRIVER=dummy;SDL_VIDEO_DRIVER=offscreen" TIMEOUT 20)
add_test(NAME audio.disk-output COMMAND audio_device_test disk-output)
set_tests_properties(audio.disk-output PROPERTIES TIMEOUT 20 ENVIRONMENT
                     "SDL_AUDIO_DRIVER=disk;SDL_AUDIO_DISK_OUTPUT_FILE=audio_device_test_disk.raw")

# Mouse and keyboard input, frame boundaries and pause() timing.
draw_test_executable(input_test SDL3::SDL3)
add_test(NAME input COMMAND input_test)
set_tests_properties(input PROPERTIES ENVIRONMENT SDL_VIDEO_DRIVER=offscreen TIMEOUT 30)

# Window lifetime.
draw_test_executable(window_test SDL3::SDL3)
add_test(NAME window.keep-open COMMAND window_test keep-open)
add_test(NAME window.image-error COMMAND window_test image-error)
set_tests_properties(window.image-error PROPERTIES
                     PASS_REGULAR_EXPRESSION "image: get: col 512 is outside the image")
set_tests_properties(window.keep-open PROPERTIES PASS_REGULAR_EXPRESSION "PASSED")
foreach(name IN ITEMS close-in-pause close-while-drawing)
  add_test(NAME window.${name} COMMAND window_test ${name})
  set_tests_properties(window.${name} PROPERTIES FAIL_REGULAR_EXPRESSION "FAILED")
endforeach()
set_tests_properties(window.keep-open window.close-in-pause window.close-while-drawing
                     window.image-error
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
  "image-get-outside|image: get: col 5 is outside the image (0 to 4)"
  "image-set-outside|image: set: row -1 is outside the image (0 to 2)"
  "image-load-missing|image: load: cannot open 'no-such-file.png'"
  "image-create-negative|image: create: width and height must not be negative"
  "image-save-format|image: save: 'out.gif' must end in .png, .jpg or .bmp"
  "image-save-empty|image: save: cannot save an empty image"
  "image-inconsistent|draw: picture: the image has 3 pixels, but width x height is 2 x 2"
  "audio-read-missing|audio: read: cannot open 'no-such-file.wav'"
  "audio-read-format|audio: read: 'music.ogg' must end in .wav or .mp3"
  "audio-read-invalid|audio: read: 'not-audio.wav' is not a valid WAV file"
  "audio-save-format|audio: save: 'out.mp3' must end in .wav"
  "audio-play-missing|audio: play: cannot open 'no-such-file.mp3'"
)
foreach(entry IN LISTS error_cases)
  string(FIND "${entry}" "|" bar)
  string(SUBSTRING "${entry}" 0 ${bar} name)
  math(EXPR start "${bar} + 1")
  string(SUBSTRING "${entry}" ${start} -1 message)
  if(NOT message MATCHES "^(draw|image|audio): ")
    set(message "draw: ${message}")
  endif()
  string(REGEX REPLACE "([][.*+?()^$])" "\\\\\\1" pattern "${message}")
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
