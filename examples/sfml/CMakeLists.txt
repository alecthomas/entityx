add_executable(sfml_example example.cc)

set(CMAKE_MODULE_PATH "/usr/share/SFML/cmake/Modules" ${CMAKE_MODULE_PATH})
find_package(SFML 2 REQUIRED COMPONENTS system window graphics)

if(SFML_FOUND)
  include_directories(${SFML_INCLUDE_DIR})
  target_link_libraries(sfml_example ${SFML_LIBRARIES})
endif(SFML_FOUND)
