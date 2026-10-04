# Tests run without a display: drawing tests use the library's headless mode,
# and window and input tests use SDL's dummy video driver, which needs no
# display or graphics library (the offscreen driver needs OpenGL, which
# macOS doesn't provide for it).

if(MSVC)
  set(CANVAS_TEST_WARNINGS /W4)
else()
  set(CANVAS_TEST_WARNINGS -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

function(canvas_test_executable name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE canvas::canvas ${ARGN})
  target_compile_options(${name} PRIVATE ${CANVAS_TEST_WARNINGS})
  # getenv and fopen are fine in tests; no "unsafe function" warnings.
  target_compile_definitions(${name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)
endfunction()

# The public headers compile cleanly, each on its own, with strict warnings
# treated as errors.
canvas_test_executable(header_test)
target_sources(header_test PRIVATE header_color_test.cpp header_image_test.cpp header_audio_test.cpp
                                   header_chance_test.cpp header_input_test.cpp header_turtle_test.cpp)
target_compile_options(header_test PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/WX,-Werror>)
add_test(NAME header COMMAND header_test)

# Snapshot tests against images in tests/reference.
canvas_test_executable(render_test)
target_include_directories(render_test SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/stb)
foreach(scene IN ITEMS shapes polygons text pictures clipping aspect image turtle)
  add_test(NAME render.${scene}
           COMMAND render_test ${scene} ${CMAKE_CURRENT_SOURCE_DIR}/reference)
  set_tests_properties(render.${scene} PROPERTIES ENVIRONMENT CANVAS_HEADLESS=1)
endforeach()

# Random numbers: the same sequence on every platform, ranges, distributions.
canvas_test_executable(chance_test)
add_test(NAME chance COMMAND chance_test)
add_test(NAME chance.seed-from-environment COMMAND chance_test env)
set_tests_properties(chance.seed-from-environment PROPERTIES ENVIRONMENT CANVAS_SEED=12345)

# Headless mode: nothing waits and no input is reported (what graders rely on).
canvas_test_executable(headless_test)
add_test(NAME headless COMMAND headless_test)
set_tests_properties(headless PROPERTIES ENVIRONMENT CANVAS_HEADLESS=1 TIMEOUT 10)

# Input helpers: fromFile with plain std::cin and the helpers on streams;
# then real standard input, connected to a file by run_with_input.cmake
# (ctest can't redirect input itself). No test reads the terminal.
canvas_test_executable(reading_test)
add_test(NAME reading COMMAND reading_test)
function(add_stdin_test name program input pattern)
  add_test(NAME ${name}
           COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:${program}>
                   -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/data/${input}
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/run_with_input.cmake)
  set_tests_properties(${name} PROPERTIES PASS_REGULAR_EXPRESSION "${pattern}")
endfunction()
canvas_test_executable(stdin_test)
add_stdin_test(reading.stdin stdin_test stdin_input.txt "age=25 name=\\[Ali Khan\\] count=5 sum=15")
canvas_test_executable(ask_test)
add_stdin_test(ask ask_test ask_input.txt "PASSED")
add_stdin_test(ask.no-more-input ask_test empty_input.txt "input: askInt: no more input")
canvas_test_executable(output_test)
add_test(NAME output COMMAND output_test)
set_tests_properties(output PROPERTIES PASS_REGULAR_EXPRESSION "PASSED")

# Turtle graphics: positions, headings, the pen.
canvas_test_executable(turtle_test)
add_test(NAME turtle COMMAND turtle_test)
set_tests_properties(turtle PROPERTIES ENVIRONMENT CANVAS_HEADLESS=1)

# text() with numbers and characters.
canvas_test_executable(text_test)
add_test(NAME text COMMAND text_test)
set_tests_properties(text PROPERTIES ENVIRONMENT CANVAS_HEADLESS=1)

# Files are also found next to the program. Run from the build folder, so the
# current folder is not the program's folder.
canvas_test_executable(lookup_test)
add_test(NAME lookup COMMAND lookup_test $<TARGET_FILE_DIR:lookup_test>
         WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
set_tests_properties(lookup PROPERTIES ENVIRONMENT CANVAS_HEADLESS=1)

# The image module and the canvas <-> image bridge.
canvas_test_executable(image_test)
add_test(NAME image COMMAND image_test)
set_tests_properties(image PROPERTIES ENVIRONMENT CANVAS_HEADLESS=1)

# The audio module without a device: files, resampling, clipping, capture.
canvas_test_executable(audio_test)
add_test(NAME audio COMMAND audio_test ${CMAKE_CURRENT_SOURCE_DIR}/data)
set_tests_properties(audio PROPERTIES
                     ENVIRONMENT "CANVAS_HEADLESS=1;CANVAS_AUDIO_CAPTURE=audio_test_capture.wav")

# The audio module with SDL's dummy (real-time, silent) and disk drivers.
canvas_test_executable(audio_device_test SDL3::SDL3)
foreach(name IN ITEMS pacing file background)
  add_test(NAME audio.${name} COMMAND audio_device_test ${name})
endforeach()
add_test(NAME audio.exit-drain COMMAND audio_device_test exit-drain)
set_tests_properties(audio.exit-drain PROPERTIES PASS_REGULAR_EXPRESSION "PASSED")
set_tests_properties(audio.pacing audio.file audio.background audio.exit-drain PROPERTIES
                     ENVIRONMENT "SDL_AUDIO_DRIVER=dummy;SDL_VIDEO_DRIVER=dummy" TIMEOUT 20)
add_test(NAME audio.close-at-exit COMMAND audio_device_test close-at-exit)
set_tests_properties(audio.close-at-exit PROPERTIES PASS_REGULAR_EXPRESSION "PASSED" TIMEOUT 20
                     ENVIRONMENT "SDL_AUDIO_DRIVER=dummy;SDL_VIDEO_DRIVER=dummy;CANVAS_AUDIO_CAPTURE=audio_device_test_close.wav")
add_test(NAME audio.disk-output COMMAND audio_device_test disk-output)
set_tests_properties(audio.disk-output PROPERTIES TIMEOUT 20 ENVIRONMENT
                     "SDL_AUDIO_DRIVER=disk;SDL_AUDIO_DISK_OUTPUT_FILE=audio_device_test_disk.raw")

# Mouse and keyboard input, frame boundaries and pause() timing.
canvas_test_executable(input_test SDL3::SDL3)
add_test(NAME input COMMAND input_test)
set_tests_properties(input PROPERTIES ENVIRONMENT SDL_VIDEO_DRIVER=dummy TIMEOUT 30)

# Window lifetime.
canvas_test_executable(window_test SDL3::SDL3)
add_test(NAME window.keep-open COMMAND window_test keep-open)
add_test(NAME window.image-error COMMAND window_test image-error)
set_tests_properties(window.image-error PROPERTIES
                     PASS_REGULAR_EXPRESSION "image: getPixel: col 512 is outside the image")
set_tests_properties(window.keep-open PROPERTIES PASS_REGULAR_EXPRESSION "PASSED")
foreach(name IN ITEMS close-in-pause close-while-drawing)
  add_test(NAME window.${name} COMMAND window_test ${name})
  set_tests_properties(window.${name} PROPERTIES FAIL_REGULAR_EXPRESSION "FAILED")
endforeach()
set_tests_properties(window.keep-open window.close-in-pause window.close-while-drawing
                     window.image-error
                     PROPERTIES ENVIRONMENT SDL_VIDEO_DRIVER=dummy TIMEOUT 10)

# Each mistake stops the program with a helpful message.
canvas_test_executable(error_test)
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
  "negative-frame-rate|setFrameRate: the frame rate must not be negative"
  "image-get-outside|image: getPixel: col 5 is outside the image (0 to 4)"
  "image-set-outside|image: setPixel: row -1 is outside the image (0 to 2)"
  "image-index-outside|image: [row][col]: row 3 is outside the image (0 to 2)"
  "image-index-const|image: [row][col]: col -1 is outside the image (0 to 4)"
  "image-load-missing|image: load: cannot open 'no-such-file.png'"
  "image-create-negative|image: create: width and height must not be negative"
  "image-save-format|image: save: 'out.gif' must end in .png, .jpg or .bmp"
  "image-save-empty|image: save: cannot save an empty image"
  "image-inconsistent|canvas: picture: the image has 3 pixels, but width x height is 2 x 2"
  "audio-read-missing|audio: read: cannot open 'no-such-file.wav'"
  "audio-read-format|audio: read: 'music.ogg' must end in .wav or .mp3"
  "audio-read-invalid|audio: read: 'not-audio.wav' is not a valid WAV file"
  "audio-save-format|audio: save: 'out.mp3' must end in .wav"
  "audio-play-missing|audio: play: cannot open 'no-such-file.mp3'"
  "input-bad-int|input: readAllInts: bad.txt, line 2: 'abc' is not a whole number"
  "input-too-large|input: readAllInts: big.txt, line 1: '99999999999' is too large for an int"
  "input-bad-double|input: readAllDoubles: line 3: '1,5' is not a number"
  "input-ask-range|input: askInt: min must not be greater than max"
  "input-to-int|input: toInt: '3.5' is not a whole number"
  "input-to-double|input: toDouble: 'abc' is not a number"
  "input-missing-file|input: fromFile: cannot open 'no-such-file.txt'"
  "output-cannot-write|output: toFile: cannot write 'no-such-folder/out.txt'"
  "turtle-nan|turtle: forward: an argument is NaN or infinite"
  "chance-empty-range|chance: uniform: the range 5 to 5 is empty (b must be greater than a)"
  "chance-bad-n|chance: uniform: n must be positive, not 0"
  "chance-bad-p|chance: bernoulli: p must be between 0 and 1"
  "chance-bad-sum|chance: discrete: probabilities must add up to 1"
  "chance-bad-seed|chance: CANVAS_SEED must be a whole number, not 'abc'"
)
foreach(entry IN LISTS error_cases)
  string(FIND "${entry}" "|" bar)
  string(SUBSTRING "${entry}" 0 ${bar} name)
  math(EXPR start "${bar} + 1")
  string(SUBSTRING "${entry}" ${start} -1 message)
  if(NOT message MATCHES "^(canvas|turtle|image|audio|chance|input|output): ")
    set(message "canvas: ${message}")
  endif()
  string(REGEX REPLACE "([][.*+?()^$])" "\\\\\\1" pattern "${message}")
  add_test(NAME error.${name} COMMAND error_test ${name})
  set_tests_properties(error.${name} PROPERTIES
                       ENVIRONMENT CANVAS_HEADLESS=1 PASS_REGULAR_EXPRESSION "${pattern}")
endforeach()
set_tests_properties(error.chance-bad-seed PROPERTIES ENVIRONMENT "CANVAS_HEADLESS=1;CANVAS_SEED=abc")

# Timings; build with `--target benchmark`, not run by ctest.
canvas_test_executable(benchmark)
set_target_properties(benchmark PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Real keyboard and mouse input, driven by manual/real_input.sh with xdotool.
# Needs a desktop and takes keyboard focus, so it is not run by ctest.
add_executable(real_input manual/real_input.cpp)
target_link_libraries(real_input PRIVATE canvas::canvas)
target_compile_options(real_input PRIVATE ${CANVAS_TEST_WARNINGS})
set_target_properties(real_input PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Sound through the real output device, recorded from its monitor and checked
# by manual/speaker_test.sh. Makes sound, so it is not run by ctest.
add_executable(speaker_test manual/speaker_test.cpp)
target_link_libraries(speaker_test PRIVATE canvas::canvas)
target_compile_options(speaker_test PRIVATE ${CANVAS_TEST_WARNINGS})
set_target_properties(speaker_test PROPERTIES EXCLUDE_FROM_ALL TRUE)
