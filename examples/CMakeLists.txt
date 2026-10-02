foreach(name IN ITEMS shapes bouncing_ball sketch image_effects)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE draw::draw)
endforeach()
