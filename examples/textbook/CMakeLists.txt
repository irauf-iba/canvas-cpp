set(textbook_examples function_graph plot_filter htree brownian chaos_game spiral drunken_turtle
                      play_that_tune bernoulli)
foreach(name IN LISTS textbook_examples)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE canvas::canvas)
endforeach()

# Each example runs headless as a test, so a change that breaks one is
# noticed. Those reading standard input get their data file through
# run_with_input.cmake, as in "./plot_filter < data/cities.txt".
if(CANVAS_BUILD_TESTS)
  foreach(name IN ITEMS function_graph htree brownian chaos_game spiral drunken_turtle bernoulli)
    add_test(NAME textbook.${name} COMMAND ${name})
  endforeach()
  foreach(entry IN ITEMS "plot_filter|plot_filter|cities.txt|1251 points"
                         "plot_filter.pakistan|plot_filter|pakistan_cities.txt|570 points"
                         "play_that_tune|play_that_tune|ode_to_joy.txt|")
    string(REPLACE "|" ";" parts "${entry}")
    list(GET parts 0 test)
    list(GET parts 1 name)
    list(GET parts 2 data)
    list(GET parts 3 pattern)
    add_test(NAME textbook.${test}
             COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:${name}>
                     -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/data/${data}
                     -P ${PROJECT_SOURCE_DIR}/tests/run_with_input.cmake)
    if(pattern)
      set_tests_properties(textbook.${test} PROPERTIES PASS_REGULAR_EXPRESSION "${pattern}")
    endif()
  endforeach()
  foreach(test IN LISTS textbook_examples ITEMS plot_filter.pakistan)
    set_tests_properties(textbook.${test} PROPERTIES ENVIRONMENT "CANVAS_HEADLESS=1;CANVAS_SEED=1" TIMEOUT 30)
  endforeach()
endif()
