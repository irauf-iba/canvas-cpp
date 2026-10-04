foreach(name IN ITEMS shapes bouncing_ball sketch image_effects scale piano random_walk
                   average grades guess koch tree)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE canvas::canvas)
endforeach()
add_subdirectory(textbook)
