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
  foreach(entry IN ITEMS "plot_filter|cities.txt|1251 points" "play_that_tune|ode_to_joy.txt|")
    string(REPLACE "|" ";" parts "${entry}")
    list(GET parts 0 name)
    list(GET parts 1 data)
    list(GET parts 2 pattern)
    add_test(NAME textbook.${name}
             COMMAND ${CMAKE_COMMAND} -DPROGRAM=$<TARGET_FILE:${name}>
                     -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/data/${data}
                     -P ${PROJECT_SOURCE_DIR}/tests/run_with_input.cmake)
    if(pattern)
      set_tests_properties(textbook.${name} PROPERTIES PASS_REGULAR_EXPRESSION "${pattern}")
    endif()
  endforeach()
  foreach(name IN LISTS textbook_examples)
    set_tests_properties(textbook.${name} PROPERTIES ENVIRONMENT "CANVAS_HEADLESS=1;CANVAS_SEED=1" TIMEOUT 30)
  endforeach()
endif()
